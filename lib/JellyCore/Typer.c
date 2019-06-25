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
        if (sourceUnit->declarations) {
            _TyperTypeNode(typer, context, node, (ASTNodeRef)sourceUnit->declarations);
        }
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

    case ASTTagLoadDirective: {
        ASTLoadDirectiveRef load = (ASTLoadDirectiveRef)node;
        _TyperTypeNode(typer, context, node, (ASTNodeRef)load->filePath);
        break;
    }

    case ASTTagBlock: {
        ASTBlockRef block = (ASTBlockRef)node;
        if (block->statements) {
            _TyperTypeNode(typer, context, node, (ASTNodeRef)block->statements);
        }
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
        unary->base.symbol          = ScopeInsertUniqueSymbol(node->scope, node->location);

        _TyperTypeNode(typer, context, node, (ASTNodeRef)unary->arguments[0]);

        SymbolRef callee   = ScopeInsertUniqueSymbol(node->scope, node->location);
        ArrayRef arguments = ArrayCreateEmpty(typer->tempAllocator, sizeof(SymbolRef), 1);
        ArrayAppendElement(arguments, &unary->arguments[0]->symbol);
        SymbolRef result         = ScopeInsertUniqueSymbol(node->scope, node->location);
        unary->base.symbol->kind = SymbolKindType;
        unary->base.symbol->type = (ASTTypeRef)ASTContextCreateApplicationType(context, node->location, callee, arguments, result);
        break;
    }

    case ASTTagBinaryExpression: {
        ASTBinaryExpressionRef binary = (ASTBinaryExpressionRef)node;
        binary->base.symbol           = ScopeInsertUniqueSymbol(node->scope, node->location);

        _TyperTypeNode(typer, context, node, (ASTNodeRef)binary->arguments[0]);
        _TyperTypeNode(typer, context, node, (ASTNodeRef)binary->arguments[1]);

        SymbolRef callee   = ScopeInsertUniqueSymbol(node->scope, node->location);
        ArrayRef arguments = ArrayCreateEmpty(typer->tempAllocator, sizeof(SymbolRef), 2);
        ArrayAppendElement(arguments, &binary->arguments[0]->symbol);
        ArrayAppendElement(arguments, &binary->arguments[1]->symbol);
        SymbolRef result          = ScopeInsertUniqueSymbol(node->scope, node->location);
        binary->base.symbol->kind = SymbolKindType;
        binary->base.symbol->type = (ASTTypeRef)ASTContextCreateApplicationType(context, node->location, callee, arguments, result);
        break;
    }

    case ASTTagIdentifierExpression: {
        ASTIdentifierExpressionRef identifier = (ASTIdentifierExpressionRef)node;
        identifier->base.symbol               = ScopeInsertUniqueSymbol(node->scope, node->location);
        identifier->base.symbol->kind         = SymbolKindType;
        identifier->base.symbol->type         = (ASTTypeRef)ASTContextCreateOpaqueType(context, node->location, identifier->name);
        break;
    }

    case ASTTagMemberAccessExpression: {
        ASTMemberAccessExpressionRef expression = (ASTMemberAccessExpressionRef)node;
        expression->base.symbol                 = ScopeInsertUniqueSymbol(node->scope, node->location);
        _TyperTypeNode(typer, context, node, (ASTNodeRef)expression->argument);
        // TODO: Create constraint type for expression->memberName
        break;
    }

    case ASTTagCallExpression: {
        ASTCallExpressionRef call = (ASTCallExpressionRef)node;
        call->base.symbol         = ScopeInsertUniqueSymbol(node->scope, node->location);

        _TyperTypeNode(typer, context, node, (ASTNodeRef)call->callee);

        if (call->arguments) {
            _TyperTypeNode(typer, context, node, (ASTNodeRef)call->arguments);
        }

        ArrayRef arguments    = ArrayCreateEmpty(typer->tempAllocator, sizeof(SymbolRef), 8);
        ASTLinkedListRef list = call->arguments;
        while (list) {
            ASTExpressionRef expression = (ASTExpressionRef)list->node;
            ArrayAppendElement(arguments, &expression->symbol);
            list = list->next;
        }

        SymbolRef result        = ScopeInsertUniqueSymbol(node->scope, node->location);
        call->base.symbol->kind = SymbolKindType;
        call->base.symbol->type = (ASTTypeRef)ASTContextCreateApplicationType(context, node->location, call->callee->symbol, arguments,
                                                                              result);
        break;
    }

    case ASTTagConstantExpression: {
        ASTConstantExpressionRef constant = (ASTConstantExpressionRef)node;
        constant->base.symbol             = ScopeInsertUniqueSymbol(node->scope, node->location);

        if (constant->kind == ASTConstantKindNil) {
            // TODO: Type constant expression
        } else if (constant->kind == ASTConstantKindBool) {
            constant->base.symbol->kind = SymbolKindType;
            constant->base.symbol->type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindBool);
        } else if (constant->kind == ASTConstantKindInt) {
            // TODO: Add support for candidate based typing of potential Int types...
            constant->base.symbol->kind = SymbolKindType;
            constant->base.symbol->type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindInt);
        } else if (constant->kind == ASTConstantKindFloat) {
            // TODO: Add support for candidate based typing of potential Float types...
            constant->base.symbol->kind = SymbolKindType;
            constant->base.symbol->type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindFloat);
        } else if (constant->kind == ASTConstantKindString) {
            // TODO: Type constant expression
        } else {
            JELLY_UNREACHABLE("Unknown kind given for ASTConstantExpression in Typer!");
        }
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
        declaration->symbol                      = ScopeInsertSymbol(node->scope, declaration->name, node->location);
        if (declaration->symbol) {
            declaration->symbol->kind = SymbolKindType;
            declaration->symbol->type = (ASTTypeRef)ASTContextCreateEnumerationType(context, node->location, declaration);
        } else {
            declaration->symbol       = ScopeInsertUniqueSymbol(node->scope, node->location);
            declaration->symbol->kind = SymbolKindType;
            declaration->symbol->type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
            ReportError("Invalid redeclaration of identifier");
        }

        if (declaration->elements) {
            _TyperTypeNode(typer, context, node, (ASTNodeRef)declaration->elements);
        }
        break;
    }

    case ASTTagFunctionDeclaration: {
        ASTFunctionDeclarationRef declaration = (ASTFunctionDeclarationRef)node;
        declaration->symbol                   = ScopeInsertSymbol(node->scope, declaration->name, node->location);
        if (declaration->parameters) {
            _TyperTypeNode(typer, context, node, (ASTNodeRef)declaration->parameters);
        }
        _TyperTypeNode(typer, context, node, (ASTNodeRef)declaration->returnType);
        _TyperTypeNode(typer, context, node, (ASTNodeRef)declaration->body);

        if (declaration->symbol) {
            ArrayRef parameters   = ArrayCreateEmpty(typer->tempAllocator, sizeof(SymbolRef), 8);
            ASTLinkedListRef list = declaration->parameters;
            while (list) {
                ArrayAppendElement(parameters, &((ASTValueDeclarationRef)list->node)->symbol);
                list = list->next;
            }

            SymbolRef result          = ScopeInsertUniqueSymbol(node->scope, node->location);
            result->kind              = SymbolKindType;
            result->type              = declaration->returnType;
            declaration->symbol->kind = SymbolKindType;
            declaration->symbol->type = (ASTTypeRef)ASTContextCreateFunctionType(context, node->location, declaration, parameters, result);
        } else {
            declaration->symbol       = ScopeInsertUniqueSymbol(node->scope, node->location);
            declaration->symbol->kind = SymbolKindType;
            declaration->symbol->type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
            ReportError("Invalid redeclaration of identifier");
        }
        break;
    }

    case ASTTagStructureDeclaration: {
        ASTStructureDeclarationRef declaration = (ASTStructureDeclarationRef)node;
        declaration->symbol                    = ScopeInsertSymbol(node->scope, declaration->name, node->location);

        if (declaration->values) {
            _TyperTypeNode(typer, context, node, (ASTNodeRef)declaration->values);
        }

        ArrayRef values       = ArrayCreateEmpty(typer->tempAllocator, sizeof(SymbolRef), 8);
        ASTLinkedListRef list = declaration->values;
        while (list) {
            ASTValueDeclarationRef value = (ASTValueDeclarationRef)list->node;
            ArrayAppendElement(values, &value->symbol);
            list = list->next;
        }

        if (declaration->symbol) {
            declaration->symbol->kind = SymbolKindType;
            declaration->symbol->type = (ASTTypeRef)ASTContextCreateStructureType(context, node->location, values);
        } else {
            declaration->symbol       = ScopeInsertUniqueSymbol(node->scope, node->location);
            declaration->symbol->kind = SymbolKindType;
            declaration->symbol->type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
            ReportError("Invalid redeclaration of identifier");
        }
        break;
    }

    case ASTTagOpaqueDeclaration:
        break;

    case ASTTagValueDeclaration: {
        ASTValueDeclarationRef declaration = (ASTValueDeclarationRef)node;
        declaration->symbol                = ScopeInsertSymbol(node->scope, declaration->name, node->location);
        if (declaration->symbol) {
            declaration->symbol->kind = SymbolKindType;
            declaration->symbol->type = declaration->type;
        } else {
            declaration->symbol       = ScopeInsertUniqueSymbol(node->scope, node->location);
            declaration->symbol->kind = SymbolKindType;
            declaration->symbol->type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
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
        type->pointee          = ScopeInsertUniqueSymbol(node->scope, node->location);
        type->pointee->kind    = SymbolKindType;
        type->pointee->type    = type->pointeeType;
        _TyperTypeNode(typer, context, node, (ASTNodeRef)type->pointeeType);
        break;
    }

    case ASTTagArrayType: {
        ASTArrayTypeRef type = (ASTArrayTypeRef)node;
        type->element        = ScopeInsertUniqueSymbol(node->scope, node->location);
        type->element->kind  = SymbolKindType;
        type->element->type  = type->elementType;
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
