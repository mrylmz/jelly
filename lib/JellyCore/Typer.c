#include "JellyCore/BumpAllocator.h"
#include "JellyCore/Diagnostic.h"
#include "JellyCore/Typer.h"

struct _Typer {
    AllocatorRef allocator;
    AllocatorRef tempAllocator;
};

static inline void _TyperTypeNode(TyperRef typer, ASTContextRef context, ASTNodeRef parent, ASTNodeRef node);

TyperRef TyperCreate(AllocatorRef allocator) {
    TyperRef typer       = (TyperRef)AllocatorAllocate(allocator, sizeof(struct _Typer));
    typer->allocator     = allocator;
    typer->tempAllocator = BumpAllocatorCreate(allocator);
    return typer;
}

void TyperDestroy(TyperRef typer) {
    AllocatorDestroy(typer->tempAllocator);
    AllocatorDeallocate(typer->allocator, typer);
}

void TyperType(TyperRef typer, ASTContextRef context, ASTNodeRef node) {
    _TyperTypeNode(typer, context, NULL, node);
}

static inline void _TyperTypeNode(TyperRef typer, ASTContextRef context, ASTNodeRef parent, ASTNodeRef node) {
    ScopeRef previousScope = SymbolTableGetCurrentScope(ASTContextGetSymbolTable(context));
    SymbolTableSetCurrentScope(ASTContextGetSymbolTable(context), node->scope);

    switch (node->tag) {
    case ASTTagSourceUnit: {
        ASTSourceUnitRef sourceUnit = (ASTSourceUnitRef)node;
        _TyperTypeNode(typer, context, node, (ASTNodeRef)sourceUnit->declarations);
        break;
    }

    case ASTTagLinkedList: {
        ASTLinkedListRef list = (ASTLinkedListRef)node;
        while (list) {
            // NOTE: We pass the parent of the list here because the list it self is a redundant information...
            _TyperTypeNode(typer, context, parent, (ASTNodeRef)list->node);
            list = list->next;
        }
        break;
    }

    case ASTTagArray: {
        ASTArrayRef array = (ASTArrayRef)node;
        for (Index index = 0; index < ASTArrayGetElementCount(array); index++) {
            _TyperTypeNode(typer, context, parent, (ASTNodeRef)ASTArrayGetElementAtIndex(array, index));
        }
        break;
    }

    case ASTTagLoadDirective: {
        ASTLoadDirectiveRef load = (ASTLoadDirectiveRef)node;
        _TyperTypeNode(typer, context, node, (ASTNodeRef)load->filePath);
        break;
    }

    case ASTTagBlock: {
        ASTBlockRef block = (ASTBlockRef)node;
        _TyperTypeNode(typer, context, node, (ASTNodeRef)block->statements);
        break;
    }

    case ASTTagIfStatement: {
        ASTIfStatementRef statement = (ASTIfStatementRef)node;
        _TyperTypeNode(typer, context, node, (ASTNodeRef)statement->condition);
        _TyperTypeNode(typer, context, node, (ASTNodeRef)statement->thenBlock);
        _TyperTypeNode(typer, context, node, (ASTNodeRef)statement->elseBlock);
        break;
    }

    case ASTTagLoopStatement: {
        ASTLoopStatementRef loop = (ASTLoopStatementRef)node;
        _TyperTypeNode(typer, context, node, (ASTNodeRef)loop->condition);
        _TyperTypeNode(typer, context, node, (ASTNodeRef)loop->loopBlock);
        break;
    }

    case ASTTagCaseStatement: {
        ASTCaseStatementRef statement = (ASTCaseStatementRef)node;
        if (statement->kind == ASTCaseKindConditional) {
            _TyperTypeNode(typer, context, node, (ASTNodeRef)statement->condition);
        }
        _TyperTypeNode(typer, context, node, (ASTNodeRef)statement->body);
        break;
    }

    case ASTTagSwitchStatement: {
        ASTSwitchStatementRef statement = (ASTSwitchStatementRef)node;
        _TyperTypeNode(typer, context, node, (ASTNodeRef)statement->argument);
        if (statement->cases) {
            _TyperTypeNode(typer, context, node, (ASTNodeRef)statement->cases);
        }
        break;
    }

    case ASTTagControlStatement: {
        ASTControlStatementRef control = (ASTControlStatementRef)node;
        if (control->kind == ASTControlKindReturn && control->result) {
            _TyperTypeNode(typer, context, node, (ASTNodeRef)control->result);
            // TODO: Add constraint that control->result has to be equal to enclosing func->result!
        }
        break;
    }

    case ASTTagUnaryExpression: {
        ASTUnaryExpressionRef unary = (ASTUnaryExpressionRef)node;
        //        unary->base.symbol          = ScopeInsertUniqueSymbol(node->scope, node);
        _TyperTypeNode(typer, context, node, (ASTNodeRef)unary->arguments[0]);
        break;
    }

    case ASTTagBinaryExpression: {
        ASTBinaryExpressionRef binary = (ASTBinaryExpressionRef)node;
        //        binary->base.symbol           = ScopeInsertUniqueSymbol(node->scope, node);
        _TyperTypeNode(typer, context, node, (ASTNodeRef)binary->arguments[0]);
        _TyperTypeNode(typer, context, node, (ASTNodeRef)binary->arguments[1]);
        break;
    }

    case ASTTagIdentifierExpression: {
        ASTIdentifierExpressionRef identifier = (ASTIdentifierExpressionRef)node;
        //        identifier->base.symbol               = ScopeInsertUniqueSymbol(node->scope, node);
        break;
    }

    case ASTTagMemberAccessExpression: {
        ASTMemberAccessExpressionRef expression = (ASTMemberAccessExpressionRef)node;
        //        expression->base.symbol                 = ScopeInsertUniqueSymbol(node->scope, node);
        _TyperTypeNode(typer, context, node, (ASTNodeRef)expression->argument);
        break;
    }

    case ASTTagCallExpression: {
        ASTCallExpressionRef call = (ASTCallExpressionRef)node;
        //        call->base.symbol         = ScopeInsertUniqueSymbol(node->scope, node);
        _TyperTypeNode(typer, context, node, (ASTNodeRef)call->callee);

        if (call->arguments) {
            _TyperTypeNode(typer, context, node, (ASTNodeRef)call->arguments);
        }
        break;
    }

    case ASTTagConstantExpression: {
        ASTConstantExpressionRef constant = (ASTConstantExpressionRef)node;
        //        constant->base.symbol             = ScopeInsertUniqueSymbol(node->scope, node);
        break;
    }

    case ASTTagModuleDeclaration: {
        ASTModuleDeclarationRef module = (ASTModuleDeclarationRef)node;
        if (module->sourceUnits) {
            _TyperTypeNode(typer, context, node, (ASTNodeRef)module->sourceUnits);
        }
        break;
    }

    case ASTTagEnumerationDeclaration: {
        ASTEnumerationDeclarationRef declaration = (ASTEnumerationDeclarationRef)node;
        SymbolRef symbol                         = ScopeInsertSymbol(node->scope, declaration->name, node);
        if (!symbol) {
            ReportError("Invalid redeclaration of identifier");
        }

        if (declaration->elements) {
            _TyperTypeNode(typer, context, node, (ASTNodeRef)declaration->elements);
        }
        break;
    }

    case ASTTagFunctionDeclaration: {
        ASTFunctionDeclarationRef declaration = (ASTFunctionDeclarationRef)node;
        SymbolRef symbol                      = ScopeInsertSymbol(node->scope, declaration->name, node);
        if (!symbol) {
            ReportError("Invalid redeclaration of identifier");
        }

        if (declaration->parameters) {
            _TyperTypeNode(typer, context, node, (ASTNodeRef)declaration->parameters);
        }
        _TyperTypeNode(typer, context, node, (ASTNodeRef)declaration->returnType);
        _TyperTypeNode(typer, context, node, (ASTNodeRef)declaration->body);
        break;
    }

    case ASTTagStructureDeclaration: {
        ASTStructureDeclarationRef declaration = (ASTStructureDeclarationRef)node;
        SymbolRef symbol                       = ScopeInsertSymbol(node->scope, declaration->name, node);
        if (!symbol) {
            ReportError("Invalid redeclaration of identifier");
        }

        if (declaration->values) {
            _TyperTypeNode(typer, context, node, (ASTNodeRef)declaration->values);
        }
        break;
    }

    case ASTTagOpaqueDeclaration:
        break;

    case ASTTagValueDeclaration: {
        ASTValueDeclarationRef declaration = (ASTValueDeclarationRef)node;
        SymbolRef symbol                   = ScopeInsertSymbol(node->scope, declaration->name, node);
        if (!symbol) {
            ReportError("Invalid redeclaration of identifier");
        }

        _TyperTypeNode(typer, context, node, (ASTNodeRef)declaration->type);
        if (declaration->initializer) {
            _TyperTypeNode(typer, context, node, (ASTNodeRef)declaration->initializer);
        }
        break;
    }

    case ASTTagPointerType: {
        ASTPointerTypeRef type = (ASTPointerTypeRef)node;
        _TyperTypeNode(typer, context, node, (ASTNodeRef)type->pointeeType);
        break;
    }

    case ASTTagArrayType: {
        ASTArrayTypeRef type = (ASTArrayTypeRef)node;
        _TyperTypeNode(typer, context, node, (ASTNodeRef)type->elementType);
        if (type->size) {
            _TyperTypeNode(typer, context, node, (ASTNodeRef)type->size);
        }
        break;
    }

    case ASTTagOpaqueType:
    case ASTTagBuiltinType:
    case ASTTagEnumerationType:
    case ASTTagFunctionType:
    case ASTTagStructureType:
    case ASTTagApplicationType:
        break;

    default:
        JELLY_UNREACHABLE("Unknown tag given for ASTNode in Typer!");
        break;
    }

    SymbolTableSetCurrentScope(ASTContextGetSymbolTable(context), previousScope);
}
