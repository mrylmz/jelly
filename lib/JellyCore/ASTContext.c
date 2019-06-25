#include "JellyCore/ASTContext.h"
#include "JellyCore/ASTNodes.h"
#include "JellyCore/BumpAllocator.h"
#include "JellyCore/SymbolTable.h"

// TODO: Add unified identifier storage

struct _ASTContext {
    AllocatorRef allocator;
    ArrayRef nodes[AST_TAG_COUNT];
    ASTModuleDeclarationRef module;
    ASTBuiltinTypeRef builtinTypes[AST_BUILTIN_TYPE_KIND_COUNT];
    SymbolTableRef symbolTable;
};

ASTNodeRef _ASTContextCreateNode(ASTContextRef context, ASTTag tag, SourceRange location);
ASTBuiltinTypeRef _ASTContextCreateBuiltinType(ASTContextRef context, SourceRange location, ASTBuiltinTypeKind builtinKind, StringRef name);
void _ASTContextInitBuiltinTypes(ASTContextRef context);

ASTContextRef ASTContextCreate(AllocatorRef allocator) {
    AllocatorRef bumpAllocator = BumpAllocatorCreate(allocator);
    ASTContextRef context      = AllocatorAllocate(bumpAllocator, sizeof(struct _ASTContext));
    context->allocator         = bumpAllocator;
    // TODO: @Bug Reallocation of dynamic arrays causes invalidation of all pointers do not store the source of truth in arrays!
    //            We can just allocate nodes dynamically without holding a reference to them because the BumpAllocator will be freed once...
    context->nodes[ASTTagLinkedList]             = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTLinkedList), 1024);
    context->nodes[ASTTagSourceUnit]             = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTSourceUnit), 1024);
    context->nodes[ASTTagLoadDirective]          = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTLoadDirective), 1024);
    context->nodes[ASTTagBlock]                  = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTBlock), 1024);
    context->nodes[ASTTagIfStatement]            = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTIfStatement), 1024);
    context->nodes[ASTTagLoopStatement]          = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTLoopStatement), 1024);
    context->nodes[ASTTagCaseStatement]          = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTCaseStatement), 1024);
    context->nodes[ASTTagSwitchStatement]        = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTSwitchStatement), 1024);
    context->nodes[ASTTagControlStatement]       = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTControlStatement), 1024);
    context->nodes[ASTTagUnaryExpression]        = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTUnaryExpression), 1024);
    context->nodes[ASTTagBinaryExpression]       = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTBinaryExpression), 1024);
    context->nodes[ASTTagIdentifierExpression]   = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTIdentifierExpression), 1024);
    context->nodes[ASTTagMemberAccessExpression] = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTMemberAccessExpression), 1024);
    context->nodes[ASTTagCallExpression]         = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTCallExpression), 1024);
    context->nodes[ASTTagConstantExpression]     = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTConstantExpression), 1024);
    context->nodes[ASTTagModuleDeclaration]      = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTModuleDeclaration), 1024);
    context->nodes[ASTTagEnumerationDeclaration] = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTEnumerationDeclaration), 1024);
    context->nodes[ASTTagFunctionDeclaration]    = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTFunctionDeclaration), 1024);
    context->nodes[ASTTagStructureDeclaration]   = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTStructureDeclaration), 1024);
    context->nodes[ASTTagOpaqueDeclaration]      = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTOpaqueDeclaration), 1024);
    context->nodes[ASTTagValueDeclaration]       = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTValueDeclaration), 1024);
    context->nodes[ASTTagOpaqueType]             = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTOpaqueType), 1024);
    context->nodes[ASTTagPointerType]            = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTPointerType), 1024);
    context->nodes[ASTTagArrayType]              = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTArrayType), 1024);
    context->nodes[ASTTagBuiltinType]            = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTBuiltinType), 1024);
    context->nodes[ASTTagEnumerationType]        = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTEnumerationType), 1024);
    context->nodes[ASTTagFunctionType]           = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTFunctionType), 1024);
    context->nodes[ASTTagStructureType]          = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTStructureType), 1024);
    context->nodes[ASTTagApplicationType]        = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTApplicationType), 1024);
    context->symbolTable                         = SymbolTableCreate(bumpAllocator);
    context->module                              = ASTContextCreateModuleDeclaration(context, SourceRangeNull(), NULL, NULL);
    _ASTContextInitBuiltinTypes(context);
    return context;
}

void ASTContextDestroy(ASTContextRef context) {
    AllocatorDestroy(context->allocator);
}

SymbolTableRef ASTContextGetSymbolTable(ASTContextRef context) {
    return context->symbolTable;
}

ASTModuleDeclarationRef ASTContextGetModule(ASTContextRef context) {
    return context->module;
}

void ASTModuleAddSourceUnit(ASTContextRef context, ASTModuleDeclarationRef module, ASTSourceUnitRef sourceUnit) {
    if (!module->sourceUnits) {
        module->sourceUnits       = (ASTLinkedListRef)_ASTContextCreateNode(context, ASTTagLinkedList, SourceRangeNull());
        module->sourceUnits->node = (ASTNodeRef)sourceUnit;
        module->sourceUnits->next = NULL;
        return;
    }

    ASTLinkedListRef list = module->sourceUnits;
    while (list->next) {
        list = list->next;
    }

    list->next       = (ASTLinkedListRef)_ASTContextCreateNode(context, ASTTagLinkedList, SourceRangeNull());
    list->next->node = (ASTNodeRef)sourceUnit;
    list->next->next = NULL;
}

ASTSourceUnitRef ASTContextCreateSourceUnit(ASTContextRef context, SourceRange location, StringRef filePath, ArrayRef declarations) {
    assert(filePath);

    ASTSourceUnitRef node = (ASTSourceUnitRef)_ASTContextCreateNode(context, ASTTagSourceUnit, location);
    node->filePath        = StringCreateCopy(context->allocator, filePath);
    node->declarations    = NULL;
    if (declarations && ArrayGetElementCount(declarations) > 0) {
        node->declarations       = (ASTLinkedListRef)_ASTContextCreateNode(context, ASTTagLinkedList, SourceRangeNull());
        node->declarations->node = *((ASTNodeRef *)ArrayGetElementAtIndex(declarations, 0));
        node->declarations->next = NULL;

        ASTLinkedListRef current = node->declarations;
        for (Index index = 1; index < ArrayGetElementCount(declarations); index++) {
            current->next       = (ASTLinkedListRef)_ASTContextCreateNode(context, ASTTagLinkedList, SourceRangeNull());
            current->next->node = *((ASTNodeRef *)ArrayGetElementAtIndex(declarations, index));
            current->next->next = NULL;
            current             = current->next;
        }
    }

    return node;
}

ASTLoadDirectiveRef ASTContextCreateLoadDirective(ASTContextRef context, SourceRange location, ASTConstantExpressionRef filePath) {
    assert(filePath && filePath->kind == ASTConstantKindString);

    ASTLoadDirectiveRef node = (ASTLoadDirectiveRef)_ASTContextCreateNode(context, ASTTagLoadDirective, location);
    node->filePath           = filePath;
    return node;
}

ASTBlockRef ASTContextCreateBlock(ASTContextRef context, SourceRange location, ArrayRef statements) {
    ASTBlockRef node = (ASTBlockRef)_ASTContextCreateNode(context, ASTTagBlock, location);
    node->statements = NULL;
    if (statements && ArrayGetElementCount(statements) > 0) {
        node->statements       = (ASTLinkedListRef)_ASTContextCreateNode(context, ASTTagLinkedList, SourceRangeNull());
        node->statements->node = *((ASTNodeRef *)ArrayGetElementAtIndex(statements, 0));
        node->statements->next = NULL;

        ASTLinkedListRef current = node->statements;
        for (Index index = 1; index < ArrayGetElementCount(statements); index++) {
            current->next       = (ASTLinkedListRef)_ASTContextCreateNode(context, ASTTagLinkedList, SourceRangeNull());
            current->next->node = *((ASTNodeRef *)ArrayGetElementAtIndex(statements, index));
            current->next->next = NULL;
            current             = current->next;
        }
    }
    return node;
}

ASTIfStatementRef ASTContextCreateIfStatement(ASTContextRef context, SourceRange location, ASTExpressionRef condition,
                                              ASTBlockRef thenBlock, ASTBlockRef elseBlock) {
    assert(condition && thenBlock && elseBlock);

    ASTIfStatementRef node = (ASTIfStatementRef)_ASTContextCreateNode(context, ASTTagIfStatement, location);
    node->condition        = condition;
    node->thenBlock        = thenBlock;
    node->elseBlock        = elseBlock;
    return node;
}

ASTLoopStatementRef ASTContextCreateLoopStatement(ASTContextRef context, SourceRange location, ASTLoopKind kind, ASTExpressionRef condition,
                                                  ASTBlockRef loopBlock) {
    assert(condition && loopBlock);

    ASTLoopStatementRef node = (ASTLoopStatementRef)_ASTContextCreateNode(context, ASTTagLoopStatement, location);
    node->kind               = kind;
    node->condition          = condition;
    node->loopBlock          = loopBlock;
    return node;
}

ASTCaseStatementRef ASTContextCreateCaseStatement(ASTContextRef context, SourceRange location, ASTCaseKind kind, ASTExpressionRef condition,
                                                  ASTBlockRef body) {
    assert((kind == ASTCaseKindElse || condition) && body);

    ASTCaseStatementRef node = (ASTCaseStatementRef)_ASTContextCreateNode(context, ASTTagCaseStatement, location);
    node->kind               = kind;
    node->condition          = condition;
    node->body               = body;
    return node;
}

ASTSwitchStatementRef ASTContextCreateSwitchStatement(ASTContextRef context, SourceRange location, ASTExpressionRef argument,
                                                      ArrayRef cases) {
    assert(argument);

    ASTSwitchStatementRef node = (ASTSwitchStatementRef)_ASTContextCreateNode(context, ASTTagSwitchStatement, location);
    node->argument             = argument;
    node->cases                = NULL;
    if (cases && ArrayGetElementCount(cases) > 0) {
        node->cases       = (ASTLinkedListRef)_ASTContextCreateNode(context, ASTTagLinkedList, SourceRangeNull());
        node->cases->node = *((ASTNodeRef *)ArrayGetElementAtIndex(cases, 0));
        node->cases->next = NULL;

        ASTLinkedListRef current = node->cases;
        for (Index index = 1; index < ArrayGetElementCount(cases); index++) {
            current->next       = (ASTLinkedListRef)_ASTContextCreateNode(context, ASTTagLinkedList, SourceRangeNull());
            current->next->node = *((ASTNodeRef *)ArrayGetElementAtIndex(cases, index));
            current->next->next = NULL;
            current             = current->next;
        }
    }
    return node;
}

ASTControlStatementRef ASTContextCreateControlStatement(ASTContextRef context, SourceRange location, ASTControlKind kind,
                                                        ASTExpressionRef result) {
    ASTControlStatementRef node = (ASTControlStatementRef)_ASTContextCreateNode(context, ASTTagControlStatement, location);
    node->kind                  = kind;
    node->result                = result;
    return node;
}

ASTUnaryExpressionRef ASTContextCreateUnaryExpression(ASTContextRef context, SourceRange location, ASTUnaryOperator op,
                                                      ASTExpressionRef arguments[1]) {
    assert(arguments[0]);

    ASTUnaryExpressionRef node = (ASTUnaryExpressionRef)_ASTContextCreateNode(context, ASTTagUnaryExpression, location);
    node->op                   = op;
    node->arguments[0]         = arguments[0];
    node->base.symbol          = NULL;
    return node;
}

ASTBinaryExpressionRef ASTContextCreateBinaryExpression(ASTContextRef context, SourceRange location, ASTBinaryOperator op,
                                                        ASTExpressionRef arguments[2]) {
    assert(arguments[0] && arguments[1]);

    ASTBinaryExpressionRef node = (ASTBinaryExpressionRef)_ASTContextCreateNode(context, ASTTagBinaryExpression, location);
    node->op                    = op;
    node->arguments[0]          = arguments[0];
    node->arguments[1]          = arguments[1];
    node->base.symbol           = NULL;
    return node;
}

ASTIdentifierExpressionRef ASTContextCreateIdentifierExpression(ASTContextRef context, SourceRange location, StringRef name) {
    assert(name);

    ASTIdentifierExpressionRef node = (ASTIdentifierExpressionRef)_ASTContextCreateNode(context, ASTTagIdentifierExpression, location);
    node->name                      = StringCreateCopy(context->allocator, name);
    node->base.symbol               = NULL;
    return node;
}

ASTMemberAccessExpressionRef ASTContextCreateMemberAccessExpression(ASTContextRef context, SourceRange location, ASTExpressionRef argument,
                                                                    StringRef memberName) {
    assert(argument && memberName);

    ASTMemberAccessExpressionRef node = (ASTMemberAccessExpressionRef)_ASTContextCreateNode(context, ASTTagMemberAccessExpression,
                                                                                            location);
    node->argument                    = argument;
    node->memberName                  = StringCreateCopy(context->allocator, memberName);
    node->base.symbol                 = NULL;
    return node;
}

ASTCallExpressionRef ASTContextCreateCallExpression(ASTContextRef context, SourceRange location, ASTExpressionRef callee,
                                                    ArrayRef arguments) {
    assert(callee);

    ASTCallExpressionRef node = (ASTCallExpressionRef)_ASTContextCreateNode(context, ASTTagCallExpression, location);
    node->callee              = callee;
    node->arguments           = NULL;
    node->base.symbol         = NULL;
    if (arguments && ArrayGetElementCount(arguments) > 0) {
        node->arguments       = (ASTLinkedListRef)_ASTContextCreateNode(context, ASTTagLinkedList, SourceRangeNull());
        node->arguments->node = *((ASTNodeRef *)ArrayGetElementAtIndex(arguments, 0));
        node->arguments->next = NULL;

        ASTLinkedListRef current = node->arguments;
        for (Index index = 1; index < ArrayGetElementCount(arguments); index++) {
            current->next       = (ASTLinkedListRef)_ASTContextCreateNode(context, ASTTagLinkedList, SourceRangeNull());
            current->next->node = *((ASTNodeRef *)ArrayGetElementAtIndex(arguments, index));
            current->next->next = NULL;
            current             = current->next;
        }
    }
    return node;
}

ASTConstantExpressionRef ASTContextCreateConstantNilExpression(ASTContextRef context, SourceRange location) {
    ASTConstantExpressionRef node = (ASTConstantExpressionRef)_ASTContextCreateNode(context, ASTTagConstantExpression, location);
    node->kind                    = ASTConstantKindNil;
    node->base.symbol             = NULL;
    return node;
}

ASTConstantExpressionRef ASTContextCreateConstantBoolExpression(ASTContextRef context, SourceRange location, Bool value) {
    ASTConstantExpressionRef node = (ASTConstantExpressionRef)_ASTContextCreateNode(context, ASTTagConstantExpression, location);
    node->kind                    = ASTConstantKindBool;
    node->boolValue               = value;
    node->base.symbol             = NULL;
    return node;
}

ASTConstantExpressionRef ASTContextCreateConstantIntExpression(ASTContextRef context, SourceRange location, UInt64 value) {
    ASTConstantExpressionRef node = (ASTConstantExpressionRef)_ASTContextCreateNode(context, ASTTagConstantExpression, location);
    node->kind                    = ASTConstantKindInt;
    node->intValue                = value;
    node->base.symbol             = NULL;
    return node;
}

ASTConstantExpressionRef ASTContextCreateConstantFloatExpression(ASTContextRef context, SourceRange location, Float64 value) {
    ASTConstantExpressionRef node = (ASTConstantExpressionRef)_ASTContextCreateNode(context, ASTTagConstantExpression, location);
    node->kind                    = ASTConstantKindFloat;
    node->floatValue              = value;
    node->base.symbol             = NULL;
    return node;
}

ASTConstantExpressionRef ASTContextCreateConstantStringExpression(ASTContextRef context, SourceRange location, StringRef value) {
    assert(value);

    ASTConstantExpressionRef node = (ASTConstantExpressionRef)_ASTContextCreateNode(context, ASTTagConstantExpression, location);
    node->kind                    = ASTConstantKindString;
    node->stringValue             = StringCreateCopy(context->allocator, value);
    node->base.symbol             = NULL;
    return node;
}

ASTModuleDeclarationRef ASTContextCreateModuleDeclaration(ASTContextRef context, SourceRange location, ArrayRef sourceUnits,
                                                          ArrayRef importedModules) {
    ASTModuleDeclarationRef node = (ASTModuleDeclarationRef)_ASTContextCreateNode(context, ASTTagModuleDeclaration, location);
    node->sourceUnits            = NULL;
    if (sourceUnits && ArrayGetElementCount(sourceUnits) > 0) {
        node->sourceUnits       = (ASTLinkedListRef)_ASTContextCreateNode(context, ASTTagLinkedList, SourceRangeNull());
        node->sourceUnits->node = *((ASTNodeRef *)ArrayGetElementAtIndex(sourceUnits, 0));
        node->sourceUnits->next = NULL;

        ASTLinkedListRef current = node->sourceUnits;
        for (Index index = 1; index < ArrayGetElementCount(sourceUnits); index++) {
            current->next       = (ASTLinkedListRef)_ASTContextCreateNode(context, ASTTagLinkedList, SourceRangeNull());
            current->next->node = *((ASTNodeRef *)ArrayGetElementAtIndex(sourceUnits, index));
            current->next->next = NULL;
            current             = current->next;
        }
    }

    node->importedModules = NULL;
    if (importedModules && ArrayGetElementCount(importedModules) > 0) {
        node->importedModules       = (ASTLinkedListRef)_ASTContextCreateNode(context, ASTTagLinkedList, SourceRangeNull());
        node->importedModules->node = *((ASTNodeRef *)ArrayGetElementAtIndex(importedModules, 0));
        node->importedModules->next = NULL;

        ASTLinkedListRef current = node->importedModules;
        for (Index index = 1; index < ArrayGetElementCount(importedModules); index++) {
            current->next       = (ASTLinkedListRef)_ASTContextCreateNode(context, ASTTagLinkedList, SourceRangeNull());
            current->next->node = *((ASTNodeRef *)ArrayGetElementAtIndex(importedModules, index));
            current->next->next = NULL;
            current             = current->next;
        }
    }

    return node;
}

ASTEnumerationDeclarationRef ASTContextCreateEnumerationDeclaration(ASTContextRef context, SourceRange location, StringRef name,
                                                                    ArrayRef elements) {
    assert(name);

    ASTEnumerationDeclarationRef node = (ASTEnumerationDeclarationRef)_ASTContextCreateNode(context, ASTTagEnumerationDeclaration,
                                                                                            location);
    node->name                        = StringCreateCopy(context->allocator, name);
    node->elements                    = NULL;
    node->symbol                      = NULL;
    if (elements && ArrayGetElementCount(elements) > 0) {
        node->elements       = (ASTLinkedListRef)_ASTContextCreateNode(context, ASTTagLinkedList, SourceRangeNull());
        node->elements->node = *((ASTNodeRef *)ArrayGetElementAtIndex(elements, 0));
        node->elements->next = NULL;

        ASTLinkedListRef current = node->elements;
        for (Index index = 1; index < ArrayGetElementCount(elements); index++) {
            current->next       = (ASTLinkedListRef)_ASTContextCreateNode(context, ASTTagLinkedList, SourceRangeNull());
            current->next->node = *((ASTNodeRef *)ArrayGetElementAtIndex(elements, index));
            current->next->next = NULL;
            current             = current->next;
        }
    }

    return node;
}

ASTFunctionDeclarationRef ASTContextCreateFunctionDeclaration(ASTContextRef context, SourceRange location, StringRef name,
                                                              ArrayRef parameters, ASTTypeRef returnType, ASTBlockRef body) {
    assert(name && returnType && body);

    ASTFunctionDeclarationRef node = (ASTFunctionDeclarationRef)_ASTContextCreateNode(context, ASTTagFunctionDeclaration, location);
    node->name                     = StringCreateCopy(context->allocator, name);
    node->parameters               = NULL;
    if (parameters && ArrayGetElementCount(parameters) > 0) {
        node->parameters       = (ASTLinkedListRef)_ASTContextCreateNode(context, ASTTagLinkedList, SourceRangeNull());
        node->parameters->node = *((ASTNodeRef *)ArrayGetElementAtIndex(parameters, 0));
        node->parameters->next = NULL;

        ASTLinkedListRef current = node->parameters;
        for (Index index = 1; index < ArrayGetElementCount(parameters); index++) {
            current->next       = (ASTLinkedListRef)_ASTContextCreateNode(context, ASTTagLinkedList, SourceRangeNull());
            current->next->node = *((ASTNodeRef *)ArrayGetElementAtIndex(parameters, index));
            current->next->next = NULL;
            current             = current->next;
        }
    }

    node->returnType = returnType;
    node->body       = body;
    node->symbol     = NULL;
    return node;
}

ASTStructureDeclarationRef ASTContextCreateStructureDeclaration(ASTContextRef context, SourceRange location, StringRef name,
                                                                ArrayRef values) {
    assert(name);

    ASTStructureDeclarationRef node = (ASTStructureDeclarationRef)_ASTContextCreateNode(context, ASTTagStructureDeclaration, location);
    node->name                      = StringCreateCopy(context->allocator, name);
    node->values                    = NULL;
    node->symbol                    = NULL;

    if (values && ArrayGetElementCount(values) > 0) {
        node->values       = (ASTLinkedListRef)_ASTContextCreateNode(context, ASTTagLinkedList, SourceRangeNull());
        node->values->node = *((ASTNodeRef *)ArrayGetElementAtIndex(values, 0));
        node->values->next = NULL;

        ASTLinkedListRef current = node->values;
        for (Index index = 1; index < ArrayGetElementCount(values); index++) {
            current->next       = (ASTLinkedListRef)_ASTContextCreateNode(context, ASTTagLinkedList, SourceRangeNull());
            current->next->node = *((ASTNodeRef *)ArrayGetElementAtIndex(values, index));
            current->next->next = NULL;
            current             = current->next;
        }
    }

    return node;
}

ASTOpaqueDeclarationRef ASTContextCreateOpaqueDeclaration(ASTContextRef context, SourceRange location, StringRef name) {
    assert(name);

    ASTOpaqueDeclarationRef node = (ASTOpaqueDeclarationRef)_ASTContextCreateNode(context, ASTTagOpaqueDeclaration, location);
    node->name                   = StringCreateCopy(context->allocator, name);
    node->symbol                 = NULL;
    return node;
}

ASTValueDeclarationRef ASTContextCreateValueDeclaration(ASTContextRef context, SourceRange location, ASTValueKind kind, StringRef name,
                                                        ASTTypeRef type, ASTExpressionRef initializer) {
    assert(name && type);
    assert((kind == ASTValueKindParameter && !initializer) || (kind == ASTValueKindVariable || kind == ASTValueKindEnumerationElement));

    ASTValueDeclarationRef node = (ASTValueDeclarationRef)_ASTContextCreateNode(context, ASTTagValueDeclaration, location);
    node->kind                  = kind;
    node->name                  = StringCreateCopy(context->allocator, name);
    node->type                  = type;
    node->initializer           = initializer;
    node->symbol                = NULL;
    return node;
}

ASTOpaqueTypeRef ASTContextCreateOpaqueType(ASTContextRef context, SourceRange location, StringRef name) {
    assert(name);

    ASTOpaqueTypeRef node = (ASTOpaqueTypeRef)_ASTContextCreateNode(context, ASTTagOpaqueType, location);
    node->name            = StringCreateCopy(context->allocator, name);
    node->declaration     = NULL;
    return node;
}

ASTPointerTypeRef ASTContextCreatePointerType(ASTContextRef context, SourceRange location, ASTTypeRef pointeeType) {
    assert(pointeeType);

    ASTPointerTypeRef node = (ASTPointerTypeRef)_ASTContextCreateNode(context, ASTTagPointerType, location);
    node->pointee          = NULL;
    node->pointeeType      = pointeeType;
    return node;
}

ASTArrayTypeRef ASTContextCreateArrayType(ASTContextRef context, SourceRange location, ASTTypeRef elementType, ASTExpressionRef size) {
    assert(elementType);

    ASTArrayTypeRef node = (ASTArrayTypeRef)_ASTContextCreateNode(context, ASTTagArrayType, location);
    node->element        = NULL;
    node->elementType    = elementType;
    node->size           = size;
    return node;
}

ASTEnumerationTypeRef ASTContextCreateEnumerationType(ASTContextRef context, SourceRange location,
                                                      ASTEnumerationDeclarationRef declaration) {
    ASTEnumerationTypeRef node = (ASTEnumerationTypeRef)_ASTContextCreateNode(context, ASTTagEnumerationType, location);
    node->declaration          = declaration;
    return node;
}

ASTFunctionTypeRef ASTContextCreateFunctionType(ASTContextRef context, SourceRange location, ASTFunctionDeclarationRef declaration,
                                                ArrayRef parameters, SymbolRef result) {
    ASTFunctionTypeRef node = (ASTFunctionTypeRef)_ASTContextCreateNode(context, ASTTagFunctionType, location);
    node->declaration       = declaration;
    node->result            = result;
    node->parameters        = NULL;
    if (parameters && ArrayGetElementCount(parameters) > 0) {
        node->parameters       = (ASTLinkedListRef)_ASTContextCreateNode(context, ASTTagLinkedList, SourceRangeNull());
        node->parameters->node = *((SymbolRef *)ArrayGetElementAtIndex(parameters, 0));
        node->parameters->next = NULL;

        ASTLinkedListRef current = node->parameters;
        for (Index index = 1; index < ArrayGetElementCount(parameters); index++) {
            current->next       = (ASTLinkedListRef)_ASTContextCreateNode(context, ASTTagLinkedList, SourceRangeNull());
            current->next->node = *((SymbolRef *)ArrayGetElementAtIndex(parameters, index));
            current->next->next = NULL;
            current             = current->next;
        }
    }
    return node;
}

ASTStructureTypeRef ASTContextCreateStructureType(ASTContextRef context, SourceRange location, ArrayRef values) {
    ASTStructureTypeRef node = (ASTStructureTypeRef)_ASTContextCreateNode(context, ASTTagStructureType, location);
    node->values             = NULL;
    if (values && ArrayGetElementCount(values) > 0) {
        node->values       = (ASTLinkedListRef)_ASTContextCreateNode(context, ASTTagLinkedList, SourceRangeNull());
        node->values->node = *((SymbolRef *)ArrayGetElementAtIndex(values, 0));
        node->values->next = NULL;

        ASTLinkedListRef current = node->values;
        for (Index index = 1; index < ArrayGetElementCount(values); index++) {
            current->next       = (ASTLinkedListRef)_ASTContextCreateNode(context, ASTTagLinkedList, SourceRangeNull());
            current->next->node = *((SymbolRef *)ArrayGetElementAtIndex(values, index));
            current->next->next = NULL;
            current             = current->next;
        }
    }
    return node;
}

ASTApplicationTypeRef ASTContextCreateApplicationType(ASTContextRef context, SourceRange location, SymbolRef callee, ArrayRef arguments,
                                                      SymbolRef result) {
    ASTApplicationTypeRef node = (ASTApplicationTypeRef)_ASTContextCreateNode(context, ASTTagApplicationType, location);
    node->callee               = callee;
    node->result               = result;
    node->arguments            = NULL;
    if (arguments && ArrayGetElementCount(arguments) > 0) {
        node->arguments       = (ASTLinkedListRef)_ASTContextCreateNode(context, ASTTagLinkedList, SourceRangeNull());
        node->arguments->node = *((SymbolRef *)ArrayGetElementAtIndex(arguments, 0));
        node->arguments->next = NULL;

        ASTLinkedListRef current = node->arguments;
        for (Index index = 1; index < ArrayGetElementCount(arguments); index++) {
            current->next       = (ASTLinkedListRef)_ASTContextCreateNode(context, ASTTagLinkedList, SourceRangeNull());
            current->next->node = *((SymbolRef *)ArrayGetElementAtIndex(arguments, index));
            current->next->next = NULL;
            current             = current->next;
        }
    }
    return node;
}

ASTBuiltinTypeRef ASTContextGetBuiltinType(ASTContextRef context, ASTBuiltinTypeKind kind) {
    return context->builtinTypes[kind];
}

ASTNodeRef _ASTContextCreateNode(ASTContextRef context, ASTTag tag, SourceRange location) {
    ASTNodeRef node = ArrayAppendUninitializedElement(context->nodes[tag]);
    node->tag       = tag;
    node->location  = location;
    node->scope     = SymbolTableGetCurrentScope(context->symbolTable); // TODO: This will not work in Typer!!!
    return node;
}

ASTBuiltinTypeRef _ASTContextCreateBuiltinType(ASTContextRef context, SourceRange location, ASTBuiltinTypeKind kind, StringRef name) {
    ASTBuiltinTypeRef node = (ASTBuiltinTypeRef)_ASTContextCreateNode(context, ASTTagBuiltinType, location);
    node->kind             = kind;
    node->name             = StringCreateCopy(context->allocator, name);
    return node;
}

// TODO: Move builtin types to a builtin module and implicitly import the module to the main module
void _ASTContextInitBuiltinTypes(ASTContextRef context) {
    SymbolTableRef symbolTable = ASTContextGetSymbolTable(context);
    ScopeRef globalScope       = SymbolTableGetGlobalScope(symbolTable);

    const Char *builtinTypeNames[AST_BUILTIN_TYPE_KIND_COUNT] = {
        "<error>", "Void",   "Bool",    "Int8", "Int16",   "Int32",   "Int64",   "Int128",  "Int",      "UInt8", "UInt16",
        "UInt32",  "UInt64", "UInt128", "UInt", "Float16", "Float32", "Float64", "Float80", "Float128", "Float",
    };

    StringRef name                                 = StringCreate(context->allocator, builtinTypeNames[ASTBuiltinTypeKindError]);
    context->builtinTypes[ASTBuiltinTypeKindError] = _ASTContextCreateBuiltinType(context, SourceRangeNull(), ASTBuiltinTypeKindError,
                                                                                  name);

    // NOTE: Iteration begins after ASTBuiltinTypeKindError which is 0 to skip addition of <error> type to the scope.
    for (Index index = ASTBuiltinTypeKindError + 1; index < AST_BUILTIN_TYPE_KIND_COUNT; index++) {
        name                         = StringCreate(context->allocator, builtinTypeNames[index]);
        context->builtinTypes[index] = _ASTContextCreateBuiltinType(context, SourceRangeNull(), (ASTBuiltinTypeKind)index, name);

        SymbolRef symbol = ScopeInsertSymbol(globalScope, name, SourceRangeNull());
        assert(symbol);
        symbol->kind = SymbolKindType;
        symbol->type = (ASTTypeRef)context->builtinTypes[index];
    }
}
