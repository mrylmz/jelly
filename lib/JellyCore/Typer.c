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

    case ASTTagBlock: {
        ASTBlockRef block = (ASTBlockRef)node;
        // NOTE: We pass the parent of the block here because the block it self is a redundant information...
        _TyperTypeNode(typer, context, parent, (ASTNodeRef)block->statements);
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
        ASTScopeInsertDeclaration(node->scope, (ASTDeclarationRef)declaration);
//        if (!symbol) {
//            ReportError("Invalid redeclaration of identifier");
//        }

        if (declaration->elements) {
            _TyperTypeNode(typer, context, node, (ASTNodeRef)declaration->elements);
        }
        break;
    }

    case ASTTagFunctionDeclaration: {
        ASTFunctionDeclarationRef declaration = (ASTFunctionDeclarationRef)node;
        ASTScopeInsertDeclaration(node->scope, (ASTDeclarationRef)declaration);
//        if (!symbol) {
//            ReportError("Invalid redeclaration of identifier");
//        }

        if (declaration->parameters) {
            _TyperTypeNode(typer, context, node, (ASTNodeRef)declaration->parameters);
        }
        _TyperTypeNode(typer, context, node, (ASTNodeRef)declaration->returnType);
        _TyperTypeNode(typer, context, node, (ASTNodeRef)declaration->body);
        break;
    }

    case ASTTagStructureDeclaration: {
        ASTStructureDeclarationRef declaration = (ASTStructureDeclarationRef)node;
        ASTScopeInsertDeclaration(node->scope, (ASTDeclarationRef)declaration);
//        if (!symbol) {
//            ReportError("Invalid redeclaration of identifier");
//        }

        if (declaration->values) {
            _TyperTypeNode(typer, context, node, (ASTNodeRef)declaration->values);
        }
        break;
    }

    case ASTTagValueDeclaration: {
        ASTValueDeclarationRef declaration = (ASTValueDeclarationRef)node;
        ASTScopeInsertDeclaration(node->scope, (ASTDeclarationRef)declaration);
//        if (!symbol) {
//            ReportError("Invalid redeclaration of identifier");
//        }

        _TyperTypeNode(typer, context, node, (ASTNodeRef)declaration->base.type);
        if (declaration->initializer) {
            _TyperTypeNode(typer, context, node, (ASTNodeRef)declaration->initializer);
        }
        break;
    }

    case ASTTagLoadDirective:
    case ASTTagIfStatement:
    case ASTTagLoopStatement:
    case ASTTagCaseStatement:
    case ASTTagSwitchStatement:
    case ASTTagControlStatement:
    case ASTTagUnaryExpression:
    case ASTTagBinaryExpression:
    case ASTTagIdentifierExpression:
    case ASTTagMemberAccessExpression:
    case ASTTagCallExpression:
    case ASTTagConstantExpression:
        break;

    case ASTTagPointerType:
    case ASTTagArrayType:
    case ASTTagOpaqueType:
    case ASTTagBuiltinType:
    case ASTTagEnumerationType:
    case ASTTagFunctionType:
    case ASTTagStructureType:
        break;

    default:
        JELLY_UNREACHABLE("Unknown tag given for ASTNode in Typer!");
        break;
    }
}
