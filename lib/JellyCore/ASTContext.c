#include "JellyCore/ASTContext.h"
#include "JellyCore/ASTNodes.h"
#include "JellyCore/BumpAllocator.h"
#include "JellyCore/SymbolTable.h"

// @Todo Add unified identifier storage
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
    //    AllocatorRef bumpAllocator                   = BumpAllocatorCreate(allocator);
    // TODO: Replace system default allocator with BumpAllocator after fixing the bugs, this will just leak a lot of memory!
    AllocatorRef bumpAllocator                   = AllocatorGetSystemDefault();
    ASTContextRef context                        = AllocatorAllocate(bumpAllocator, sizeof(struct _ASTContext));
    context->allocator                           = bumpAllocator;
    context->nodes[ASTTagSourceUnit]             = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTSourceUnit), 8);
    context->nodes[ASTTagLoadDirective]          = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTLoadDirective), 8);
    context->nodes[ASTTagBlock]                  = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTBlock), 8);
    context->nodes[ASTTagIfStatement]            = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTIfStatement), 8);
    context->nodes[ASTTagLoopStatement]          = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTLoopStatement), 8);
    context->nodes[ASTTagCaseStatement]          = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTCaseStatement), 8);
    context->nodes[ASTTagSwitchStatement]        = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTSwitchStatement), 8);
    context->nodes[ASTTagControlStatement]       = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTControlStatement), 8);
    context->nodes[ASTTagUnaryExpression]        = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTUnaryExpression), 8);
    context->nodes[ASTTagBinaryExpression]       = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTBinaryExpression), 8);
    context->nodes[ASTTagIdentifierExpression]   = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTIdentifierExpression), 8);
    context->nodes[ASTTagMemberAccessExpression] = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTMemberAccessExpression), 8);
    context->nodes[ASTTagCallExpression]         = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTCallExpression), 8);
    context->nodes[ASTTagConstantExpression]     = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTConstantExpression), 8);
    context->nodes[ASTTagModuleDeclaration]      = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTModuleDeclaration), 8);
    context->nodes[ASTTagEnumerationDeclaration] = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTEnumerationDeclaration), 8);
    context->nodes[ASTTagFunctionDeclaration]    = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTFunctionDeclaration), 8);
    context->nodes[ASTTagStructureDeclaration]   = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTStructureDeclaration), 8);
    context->nodes[ASTTagOpaqueDeclaration]      = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTOpaqueDeclaration), 8);
    context->nodes[ASTTagValueDeclaration]       = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTValueDeclaration), 8);
    context->nodes[ASTTagOpaqueType]             = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTOpaqueType), 8);
    context->nodes[ASTTagPointerType]            = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTPointerType), 8);
    context->nodes[ASTTagArrayType]              = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTArrayType), 8);
    context->nodes[ASTTagBuiltinType]            = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTBuiltinType), 8);
    context->module                              = ASTContextCreateModuleDeclaration(context, SourceRangeNull(), NULL, NULL);
    context->symbolTable                         = SymbolTableCreate(bumpAllocator);
    _ASTContextInitBuiltinTypes(context);
    return context;
}

void ASTContextDestroy(ASTContextRef context) {
    // TODO: This is currently commented out because the BumpAllocator is not used, readd this after fixing the bugs...
    //    AllocatorDestroy(context->allocator);
}

SymbolTableRef ASTContextGetSymbolTable(ASTContextRef context) {
    return context->symbolTable;
}

ASTModuleDeclarationRef ASTContextGetModule(ASTContextRef context) {
    return context->module;
}

ASTSourceUnitRef ASTContextCreateSourceUnit(ASTContextRef context, SourceRange location, StringRef filePath, ArrayRef declarations) {
    assert(filePath);

    ASTSourceUnitRef node = (ASTSourceUnitRef)_ASTContextCreateNode(context, ASTTagSourceUnit, location);
    node->filePath        = StringCreateCopy(context->allocator, filePath);
    node->declarations    = declarations ? ArrayCreateCopy(context->allocator, declarations)
                                      : ArrayCreateEmpty(context->allocator, sizeof(ASTNodeRef), 8);
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
    node->statements = statements ? ArrayCreateCopy(context->allocator, statements)
                                  : ArrayCreateEmpty(context->allocator, sizeof(ASTNodeRef), 8);
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
    node->cases = cases ? ArrayCreateCopy(context->allocator, cases) : ArrayCreateEmpty(context->allocator, sizeof(ASTNodeRef), 8);
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
    return node;
}

ASTBinaryExpressionRef ASTContextCreateBinaryExpression(ASTContextRef context, SourceRange location, ASTBinaryOperator op,
                                                        ASTExpressionRef arguments[2]) {
    assert(arguments[0] && arguments[1]);

    ASTBinaryExpressionRef node = (ASTBinaryExpressionRef)_ASTContextCreateNode(context, ASTTagBinaryExpression, location);
    node->op                    = op;
    node->arguments[0]          = arguments[0];
    node->arguments[1]          = arguments[1];
    return node;
}

ASTIdentifierExpressionRef ASTContextCreateIdentifierExpression(ASTContextRef context, SourceRange location, StringRef name) {
    assert(name);

    ASTIdentifierExpressionRef node = (ASTIdentifierExpressionRef)_ASTContextCreateNode(context, ASTTagIdentifierExpression, location);
    node->name                      = StringCreateCopy(context->allocator, name);
    return node;
}

ASTMemberAccessExpressionRef ASTContextCreateMemberAccessExpression(ASTContextRef context, SourceRange location, ASTExpressionRef argument,
                                                                    StringRef memberName) {
    assert(argument && memberName);

    ASTMemberAccessExpressionRef node = (ASTMemberAccessExpressionRef)_ASTContextCreateNode(context, ASTTagMemberAccessExpression,
                                                                                            location);
    node->argument                    = argument;
    node->memberName                  = StringCreateCopy(context->allocator, memberName);
    return node;
}

ASTCallExpressionRef ASTContextCreateCallExpression(ASTContextRef context, SourceRange location, ASTExpressionRef callee,
                                                    ArrayRef arguments) {
    assert(callee);

    ASTCallExpressionRef node = (ASTCallExpressionRef)_ASTContextCreateNode(context, ASTTagCallExpression, location);
    node->callee              = callee;
    node->arguments           = arguments ? ArrayCreateCopy(context->allocator, arguments)
                                : ArrayCreateEmpty(context->allocator, sizeof(ASTNodeRef), 8);
    return node;
}

ASTConstantExpressionRef ASTContextCreateConstantNilExpression(ASTContextRef context, SourceRange location) {
    ASTConstantExpressionRef node = (ASTConstantExpressionRef)_ASTContextCreateNode(context, ASTTagConstantExpression, location);
    node->kind                    = ASTConstantKindNil;
    return node;
}

ASTConstantExpressionRef ASTContextCreateConstantBoolExpression(ASTContextRef context, SourceRange location, Bool value) {
    ASTConstantExpressionRef node = (ASTConstantExpressionRef)_ASTContextCreateNode(context, ASTTagConstantExpression, location);
    node->kind                    = ASTConstantKindBool;
    node->boolValue               = value;
    return node;
}

ASTConstantExpressionRef ASTContextCreateConstantIntExpression(ASTContextRef context, SourceRange location, UInt64 value) {
    ASTConstantExpressionRef node = (ASTConstantExpressionRef)_ASTContextCreateNode(context, ASTTagConstantExpression, location);
    node->kind                    = ASTConstantKindInt;
    node->intValue                = value;
    return node;
}

ASTConstantExpressionRef ASTContextCreateConstantFloatExpression(ASTContextRef context, SourceRange location, Float64 value) {
    ASTConstantExpressionRef node = (ASTConstantExpressionRef)_ASTContextCreateNode(context, ASTTagControlStatement, location);
    node->kind                    = ASTConstantKindFloat;
    node->floatValue              = value;
    return node;
}

ASTConstantExpressionRef ASTContextCreateConstantStringExpression(ASTContextRef context, SourceRange location, StringRef value) {
    assert(value);

    ASTConstantExpressionRef node = (ASTConstantExpressionRef)_ASTContextCreateNode(context, ASTTagConstantExpression, location);
    node->kind                    = ASTConstantKindString;
    node->stringValue             = StringCreateCopy(context->allocator, value);
    return node;
}

ASTModuleDeclarationRef ASTContextCreateModuleDeclaration(ASTContextRef context, SourceRange location, ArrayRef sourceUnits,
                                                          ArrayRef importedModules) {
    ASTModuleDeclarationRef node = (ASTModuleDeclarationRef)_ASTContextCreateNode(context, ASTTagModuleDeclaration, location);
    node->sourceUnits            = sourceUnits ? ArrayCreateCopy(context->allocator, sourceUnits)
                                    : ArrayCreateEmpty(context->allocator, sizeof(ASTNodeRef), 8);
    node->importedModules = importedModules ? ArrayCreateCopy(context->allocator, importedModules)
                                            : ArrayCreateEmpty(context->allocator, sizeof(ASTNodeRef), 8);
    return node;
}

ASTEnumerationDeclarationRef ASTContextCreateEnumerationDeclaration(ASTContextRef context, SourceRange location, StringRef name,
                                                                    ArrayRef elements) {
    assert(name);

    ASTEnumerationDeclarationRef node = (ASTEnumerationDeclarationRef)_ASTContextCreateNode(context, ASTTagEnumerationDeclaration,
                                                                                            location);
    node->name                        = StringCreateCopy(context->allocator, name);
    node->elements = elements ? ArrayCreateCopy(context->allocator, elements) : ArrayCreateEmpty(context->allocator, sizeof(ASTNodeRef), 8);
    return node;
}

ASTFunctionDeclarationRef ASTContextCreateFunctionDeclaration(ASTContextRef context, SourceRange location, StringRef name,
                                                              ArrayRef parameters, ASTTypeRef returnType, ASTBlockRef body) {
    assert(name && returnType && body);

    ASTFunctionDeclarationRef node = (ASTFunctionDeclarationRef)_ASTContextCreateNode(context, ASTTagFunctionDeclaration, location);
    node->name                     = StringCreateCopy(context->allocator, name);
    node->parameters               = parameters ? ArrayCreateCopy(context->allocator, parameters)
                                  : ArrayCreateEmpty(context->allocator, sizeof(ASTNodeRef), 8);
    node->returnType = returnType;
    node->body       = body;
    return node;
}

ASTStructureDeclarationRef ASTContextCreateStructureDeclaration(ASTContextRef context, SourceRange location, StringRef name,
                                                                ArrayRef values) {
    assert(name);

    ASTStructureDeclarationRef node = (ASTStructureDeclarationRef)_ASTContextCreateNode(context, ASTTagStructureDeclaration, location);
    node->name                      = StringCreateCopy(context->allocator, name);
    node->values = values ? ArrayCreateCopy(context->allocator, values) : ArrayCreateEmpty(context->allocator, sizeof(ASTNodeRef), 8);
    return node;
}

ASTOpaqueDeclarationRef ASTContextCreateOpaqueDeclaration(ASTContextRef context, SourceRange location, StringRef name) {
    assert(name);

    ASTOpaqueDeclarationRef node = (ASTOpaqueDeclarationRef)_ASTContextCreateNode(context, ASTTagOpaqueDeclaration, location);
    node->name                   = StringCreateCopy(context->allocator, name);
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
    node->kind            = ASTTypeKindOpaque;
    node->name            = StringCreateCopy(context->allocator, name);
    return node;
}

ASTPointerTypeRef ASTContextCreatePointerType(ASTContextRef context, SourceRange location, ASTTypeRef pointeeType) {
    assert(pointeeType);

    ASTPointerTypeRef node = (ASTPointerTypeRef)_ASTContextCreateNode(context, ASTTagPointerType, location);
    node->kind             = ASTTypeKindPointer;
    node->pointeeType      = pointeeType;
    return node;
}

ASTArrayTypeRef ASTContextCreateArrayType(ASTContextRef context, SourceRange location, ASTTypeRef elementType, ASTExpressionRef size) {
    assert(elementType);

    ASTArrayTypeRef node = (ASTArrayTypeRef)_ASTContextCreateNode(context, ASTTagArrayType, location);
    node->kind           = ASTTypeKindArray;
    node->elementType    = elementType;
    node->size           = size;
    return node;
}

ASTBuiltinTypeRef ASTContextGetBuiltinType(ASTContextRef context, ASTBuiltinTypeKind kind) {
    return context->builtinTypes[kind];
}

ASTNodeRef _ASTContextCreateNode(ASTContextRef context, ASTTag tag, SourceRange location) {
    ASTNodeRef node = ArrayAppendUninitializedElement(context->nodes[tag]);
    node->tag       = tag;
    node->location  = location;
    return node;
}

ASTBuiltinTypeRef _ASTContextCreateBuiltinType(ASTContextRef context, SourceRange location, ASTBuiltinTypeKind builtinKind,
                                               StringRef name) {
    ASTBuiltinTypeRef node = (ASTBuiltinTypeRef)_ASTContextCreateNode(context, ASTTagBuiltinType, location);
    node->builtinKind      = builtinKind;
    node->name             = StringCreateCopy(context->allocator, name);
    return node;
}

void _ASTContextInitBuiltinTypes(ASTContextRef context) {
    SymbolTableRef symbolTable = ASTContextGetSymbolTable(context);
    ScopeRef globalScope       = SymbolTableGetGlobalScope(symbolTable);

    const Char *builtinTypeNames[AST_BUILTIN_TYPE_KIND_COUNT] = {
        "<error>", "Void",   "Bool",    "Int8", "Int16",   "Int32",   "Int64",   "Int128",  "Int",      "UInt8", "UInt16",
        "UInt32",  "UInt64", "UInt128", "UInt", "Float16", "Float32", "Float64", "Float80", "Float128", "Float",
    };

    for (Index index = 0; index < AST_BUILTIN_TYPE_KIND_COUNT; index++) {
        StringRef name               = StringCreate(context->allocator, builtinTypeNames[index]);
        context->builtinTypes[index] = _ASTContextCreateBuiltinType(context, SourceRangeNull(), (ASTBuiltinTypeKind)index, name);

        SymbolRef symbol = ScopeInsertSymbol(globalScope, name, SourceRangeNull());
        assert(symbol);
    }
}
