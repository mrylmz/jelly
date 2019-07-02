#include "JellyCore/ASTContext.h"
#include "JellyCore/ASTNodes.h"
#include "JellyCore/BumpAllocator.h"
#include "JellyCore/SymbolTable.h"

// TODO: Add unified identifier storage

struct _ASTContext {
    AllocatorRef allocator;
    ArrayRef nodes[AST_TAG_COUNT];
    ASTModuleDeclarationRef module;
    SymbolTableRef symbolTable;

    ASTModuleDeclarationRef builtinModule;
    ASTBuiltinTypeRef builtinTypes[AST_BUILTIN_TYPE_KIND_COUNT];
};

ASTNodeRef _ASTContextCreateNode(ASTContextRef context, ASTTag tag, SourceRange location);
ASTBuiltinTypeRef _ASTContextCreateBuiltinType(ASTContextRef context, SourceRange location, ASTBuiltinTypeKind builtinKind);
void _ASTContextInitBuiltinTypes(ASTContextRef context);

ASTContextRef ASTContextCreate(AllocatorRef allocator) {
    AllocatorRef bumpAllocator = BumpAllocatorCreate(allocator);
    ASTContextRef context      = AllocatorAllocate(bumpAllocator, sizeof(struct _ASTContext));
    context->allocator         = bumpAllocator;
    // TODO: @Bug Reallocation of dynamic arrays causes invalidation of all pointers do not store the source of truth in arrays!
    //            We can just allocate nodes dynamically without holding a reference to them because the BumpAllocator will be freed once...
    context->nodes[ASTTagSourceUnit]             = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTSourceUnit), 1024);
    context->nodes[ASTTagLinkedList]             = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTLinkedList), 1024);
    context->nodes[ASTTagArray]                  = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTArray), 1024);
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
    context->symbolTable                         = SymbolTableCreate(context->allocator);
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
    ASTArrayAppendElement(module->sourceUnits, sourceUnit);
}

ASTSourceUnitRef ASTContextCreateSourceUnit(ASTContextRef context, SourceRange location, StringRef filePath, ArrayRef declarations) {
    assert(filePath);

    ASTSourceUnitRef node = (ASTSourceUnitRef)_ASTContextCreateNode(context, ASTTagSourceUnit, location);
    node->filePath        = StringCreateCopy(context->allocator, filePath);
    node->declarations    = ASTContextCreateArray(context, location);
    if (declarations) {
        ASTArrayAppendArray(node->declarations, declarations);
    }
    return node;
}

ASTLinkedListRef ASTContextCreateLinkedList(ASTContextRef context, SourceRange location) {
    ASTLinkedListRef list = (ASTLinkedListRef)_ASTContextCreateNode(context, ASTTagLinkedList, location);
    list->node            = NULL;
    list->next            = NULL;
    return list;
}

ASTArrayRef ASTContextCreateArray(ASTContextRef context, SourceRange location) {
    ASTArrayRef array   = (ASTArrayRef)_ASTContextCreateNode(context, ASTTagArray, location);
    array->context      = context;
    array->elementCount = 0;
    array->list         = NULL;
    return array;
}

ASTLoadDirectiveRef ASTContextCreateLoadDirective(ASTContextRef context, SourceRange location, ASTConstantExpressionRef filePath) {
    assert(filePath && filePath->kind == ASTConstantKindString);

    ASTLoadDirectiveRef node = (ASTLoadDirectiveRef)_ASTContextCreateNode(context, ASTTagLoadDirective, location);
    node->filePath           = filePath;
    return node;
}

ASTBlockRef ASTContextCreateBlock(ASTContextRef context, SourceRange location, ArrayRef statements) {
    ASTBlockRef node = (ASTBlockRef)_ASTContextCreateNode(context, ASTTagBlock, location);
    node->statements = ASTContextCreateArray(context, location);
    if (statements) {
        ASTArrayAppendArray(node->statements, statements);
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
    node->cases                = ASTContextCreateArray(context, location);
    if (cases) {
        ASTArrayAppendArray(node->cases, cases);
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
    node->base.type            = NULL;
    return node;
}

ASTBinaryExpressionRef ASTContextCreateBinaryExpression(ASTContextRef context, SourceRange location, ASTBinaryOperator op,
                                                        ASTExpressionRef arguments[2]) {
    assert(arguments[0] && arguments[1]);

    ASTBinaryExpressionRef node = (ASTBinaryExpressionRef)_ASTContextCreateNode(context, ASTTagBinaryExpression, location);
    node->op                    = op;
    node->arguments[0]          = arguments[0];
    node->arguments[1]          = arguments[1];
    node->base.type             = NULL;
    return node;
}

ASTIdentifierExpressionRef ASTContextCreateIdentifierExpression(ASTContextRef context, SourceRange location, StringRef name) {
    assert(name);

    ASTIdentifierExpressionRef node = (ASTIdentifierExpressionRef)_ASTContextCreateNode(context, ASTTagIdentifierExpression, location);
    node->name                      = StringCreateCopy(context->allocator, name);
    node->base.type                 = NULL;
    node->resolvedDeclaration       = NULL;
    return node;
}

ASTMemberAccessExpressionRef ASTContextCreateMemberAccessExpression(ASTContextRef context, SourceRange location, ASTExpressionRef argument,
                                                                    StringRef memberName) {
    assert(argument && memberName);

    ASTMemberAccessExpressionRef node = (ASTMemberAccessExpressionRef)_ASTContextCreateNode(context, ASTTagMemberAccessExpression,
                                                                                            location);
    node->argument                    = argument;
    node->memberName                  = StringCreateCopy(context->allocator, memberName);
    node->pointerDepth                = 0;
    node->base.type                   = NULL;
    return node;
}

ASTCallExpressionRef ASTContextCreateCallExpression(ASTContextRef context, SourceRange location, ASTExpressionRef callee,
                                                    ArrayRef arguments) {
    assert(callee);

    ASTCallExpressionRef node = (ASTCallExpressionRef)_ASTContextCreateNode(context, ASTTagCallExpression, location);
    node->callee              = callee;
    node->arguments           = ASTContextCreateArray(context, location);
    node->base.type           = NULL;
    if (arguments) {
        ASTArrayAppendArray(node->arguments, arguments);
    }
    return node;
}

ASTConstantExpressionRef ASTContextCreateConstantNilExpression(ASTContextRef context, SourceRange location) {
    ASTConstantExpressionRef node = (ASTConstantExpressionRef)_ASTContextCreateNode(context, ASTTagConstantExpression, location);
    node->kind                    = ASTConstantKindNil;
    node->base.type               = NULL;
    return node;
}

ASTConstantExpressionRef ASTContextCreateConstantBoolExpression(ASTContextRef context, SourceRange location, Bool value) {
    ASTConstantExpressionRef node = (ASTConstantExpressionRef)_ASTContextCreateNode(context, ASTTagConstantExpression, location);
    node->kind                    = ASTConstantKindBool;
    node->boolValue               = value;
    node->base.type               = NULL;
    return node;
}

ASTConstantExpressionRef ASTContextCreateConstantIntExpression(ASTContextRef context, SourceRange location, UInt64 value) {
    ASTConstantExpressionRef node = (ASTConstantExpressionRef)_ASTContextCreateNode(context, ASTTagConstantExpression, location);
    node->kind                    = ASTConstantKindInt;
    node->intValue                = value;
    node->base.type               = NULL;
    return node;
}

ASTConstantExpressionRef ASTContextCreateConstantFloatExpression(ASTContextRef context, SourceRange location, Float64 value) {
    ASTConstantExpressionRef node = (ASTConstantExpressionRef)_ASTContextCreateNode(context, ASTTagConstantExpression, location);
    node->kind                    = ASTConstantKindFloat;
    node->floatValue              = value;
    node->base.type               = NULL;
    return node;
}

ASTConstantExpressionRef ASTContextCreateConstantStringExpression(ASTContextRef context, SourceRange location, StringRef value) {
    assert(value);

    ASTConstantExpressionRef node = (ASTConstantExpressionRef)_ASTContextCreateNode(context, ASTTagConstantExpression, location);
    node->kind                    = ASTConstantKindString;
    node->stringValue             = StringCreateCopy(context->allocator, value);
    node->base.type               = NULL;
    return node;
}

ASTModuleDeclarationRef ASTContextCreateModuleDeclaration(ASTContextRef context, SourceRange location, ArrayRef sourceUnits,
                                                          ArrayRef importedModules) {
    ASTModuleDeclarationRef node = (ASTModuleDeclarationRef)_ASTContextCreateNode(context, ASTTagModuleDeclaration, location);
    node->sourceUnits            = ASTContextCreateArray(context, location);
    node->importedModules        = ASTContextCreateArray(context, location);
    if (sourceUnits) {
        ASTArrayAppendArray(node->sourceUnits, sourceUnits);
    }
    if (importedModules) {
        ASTArrayAppendArray(node->importedModules, importedModules);
    }
    return node;
}

ASTEnumerationDeclarationRef ASTContextCreateEnumerationDeclaration(ASTContextRef context, SourceRange location, StringRef name,
                                                                    ArrayRef elements) {
    assert(name);

    ASTEnumerationDeclarationRef node = (ASTEnumerationDeclarationRef)_ASTContextCreateNode(context, ASTTagEnumerationDeclaration,
                                                                                            location);
    node->name                        = StringCreateCopy(context->allocator, name);
    node->elements                    = ASTContextCreateArray(context, location);
    if (elements) {
        ASTArrayAppendArray(node->elements, elements);
    }
    node->type = (ASTTypeRef)ASTContextCreateEnumerationType(context, location, node);
    return node;
}

ASTFunctionDeclarationRef ASTContextCreateFunctionDeclaration(ASTContextRef context, SourceRange location, ASTFixity fixity, StringRef name,
                                                              ArrayRef parameters, ASTTypeRef returnType, ASTBlockRef body) {
    assert(name && returnType && body);

    ASTFunctionDeclarationRef node = (ASTFunctionDeclarationRef)_ASTContextCreateNode(context, ASTTagFunctionDeclaration, location);
    node->fixity                   = ASTFixityNone;
    node->name                     = StringCreateCopy(context->allocator, name);
    node->parameters               = ASTContextCreateArray(context, location);
    node->returnType               = returnType;
    node->body                     = body;
    if (parameters) {
        ASTArrayAppendArray(node->parameters, parameters);
    }
    node->type        = (ASTTypeRef)ASTContextCreateFunctionType(context, location, node);
    node->foreign     = false;
    node->foreignName = NULL;
    return node;
}

ASTFunctionDeclarationRef ASTContextCreateForeignFunctionDeclaration(ASTContextRef context, SourceRange location, ASTFixity fixity,
                                                                     StringRef name, ArrayRef parameters, ASTTypeRef returnType,
                                                                     StringRef foreignName) {
    assert(name && returnType && foreignName);

    ASTFunctionDeclarationRef node = (ASTFunctionDeclarationRef)_ASTContextCreateNode(context, ASTTagFunctionDeclaration, location);
    node->fixity                   = ASTFixityNone;
    node->name                     = StringCreateCopy(context->allocator, name);
    node->parameters               = ASTContextCreateArray(context, location);
    node->returnType               = returnType;
    node->body                     = NULL;
    if (parameters) {
        ASTArrayAppendArray(node->parameters, parameters);
    }
    node->type        = (ASTTypeRef)ASTContextCreateFunctionType(context, location, node);
    node->foreign     = true;
    node->foreignName = StringCreateCopy(context->allocator, foreignName);
    return node;
}

ASTStructureDeclarationRef ASTContextCreateStructureDeclaration(ASTContextRef context, SourceRange location, StringRef name,
                                                                ArrayRef values) {
    assert(name);

    ASTStructureDeclarationRef node = (ASTStructureDeclarationRef)_ASTContextCreateNode(context, ASTTagStructureDeclaration, location);
    node->name                      = StringCreateCopy(context->allocator, name);
    node->values                    = ASTContextCreateArray(context, location);
    node->innerScope                = NULL;
    if (values) {
        ASTArrayAppendArray(node->values, values);
    }
    node->type = (ASTTypeRef)ASTContextCreateStructureType(context, location, node);
    return node;
}

ASTOpaqueDeclarationRef ASTContextCreateOpaqueDeclaration(ASTContextRef context, SourceRange location, StringRef name, ASTTypeRef type) {
    assert(name && type);

    ASTOpaqueDeclarationRef node = (ASTOpaqueDeclarationRef)_ASTContextCreateNode(context, ASTTagOpaqueDeclaration, location);
    node->name                   = StringCreateCopy(context->allocator, name);
    node->type                   = type;
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
    return node;
}

ASTOpaqueTypeRef ASTContextCreateOpaqueType(ASTContextRef context, SourceRange location, StringRef name) {
    assert(name);

    ASTOpaqueTypeRef node = (ASTOpaqueTypeRef)_ASTContextCreateNode(context, ASTTagOpaqueType, location);
    node->name            = StringCreateCopy(context->allocator, name);
    return node;
}

ASTPointerTypeRef ASTContextCreatePointerType(ASTContextRef context, SourceRange location, ASTTypeRef pointeeType) {
    assert(pointeeType);

    ASTPointerTypeRef node = (ASTPointerTypeRef)_ASTContextCreateNode(context, ASTTagPointerType, location);
    node->pointeeType      = pointeeType;
    return node;
}

ASTArrayTypeRef ASTContextCreateArrayType(ASTContextRef context, SourceRange location, ASTTypeRef elementType, ASTExpressionRef size) {
    assert(elementType);

    ASTArrayTypeRef node = (ASTArrayTypeRef)_ASTContextCreateNode(context, ASTTagArrayType, location);
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

ASTFunctionTypeRef ASTContextCreateFunctionType(ASTContextRef context, SourceRange location, ASTFunctionDeclarationRef declaration) {
    ASTFunctionTypeRef node = (ASTFunctionTypeRef)_ASTContextCreateNode(context, ASTTagFunctionType, location);
    node->declaration       = declaration;
    return node;
}

ASTStructureTypeRef ASTContextCreateStructureType(ASTContextRef context, SourceRange location, ASTStructureDeclarationRef declaration) {
    ASTStructureTypeRef node = (ASTStructureTypeRef)_ASTContextCreateNode(context, ASTTagStructureType, location);
    node->declaration        = declaration;
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

ASTBuiltinTypeRef _ASTContextCreateBuiltinType(ASTContextRef context, SourceRange location, ASTBuiltinTypeKind kind) {
    ASTBuiltinTypeRef node = (ASTBuiltinTypeRef)_ASTContextCreateNode(context, ASTTagBuiltinType, location);
    node->kind             = kind;
    return node;
}

// TODO: Move builtin types to a builtin module and implicitly import the module to the main module
void _ASTContextInitBuiltinTypes(ASTContextRef context) {
    SymbolTableRef symbolTable = ASTContextGetSymbolTable(context);
    ScopeRef globalScope       = SymbolTableGetGlobalScope(symbolTable);

    const Char *builtinTypeNames[AST_BUILTIN_TYPE_KIND_COUNT] = {
        "<error>", "Void",   "Bool",   "Int8",   "Int16", "Int32",   "Int64",   "Int",
        "UInt8",   "UInt16", "UInt32", "UInt64", "UInt",  "Float32", "Float64", "Float",
    };

    StringRef name                                 = StringCreate(context->allocator, builtinTypeNames[ASTBuiltinTypeKindError]);
    context->builtinTypes[ASTBuiltinTypeKindError] = _ASTContextCreateBuiltinType(context, SourceRangeNull(), ASTBuiltinTypeKindError);

    // NOTE: Iteration begins after ASTBuiltinTypeKindError which is 0 to skip addition of <error> type to the scope.
    for (Index index = ASTBuiltinTypeKindError + 1; index < AST_BUILTIN_TYPE_KIND_COUNT; index++) {
        name                                = StringCreate(context->allocator, builtinTypeNames[index]);
        context->builtinTypes[index]        = _ASTContextCreateBuiltinType(context, SourceRangeNull(), (ASTBuiltinTypeKind)index);
        ASTOpaqueDeclarationRef declaration = ASTContextCreateOpaqueDeclaration(context, SourceRangeNull(), name,
                                                                                (ASTTypeRef)context->builtinTypes[ASTBuiltinTypeKindError]);
        SymbolRef symbol                    = ScopeInsertSymbol(globalScope, name, (ASTNodeRef)declaration);
        assert(symbol);
    }
}
