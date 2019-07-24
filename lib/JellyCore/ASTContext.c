#include "JellyCore/ASTContext.h"
#include "JellyCore/ASTMangling.h"
#include "JellyCore/ASTNodes.h"
#include "JellyCore/ASTScope.h"
#include "JellyCore/BumpAllocator.h"

// TODO: Add unified identifier storage

struct _ASTContext {
    AllocatorRef allocator;
    ArrayRef nodes[AST_TAG_COUNT];
    ASTModuleDeclarationRef module;

    ASTModuleDeclarationRef builtinModule;
    ASTBuiltinTypeRef builtinTypes[AST_BUILTIN_TYPE_KIND_COUNT];
    ASTStructureTypeRef stringType;
};

ASTNodeRef _ASTContextCreateNode(ASTContextRef context, ASTTag tag, SourceRange location, ASTScopeRef scope);
ASTBuiltinTypeRef _ASTContextCreateBuiltinType(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                               ASTBuiltinTypeKind builtinKind);
void _ASTContextInitBuiltinTypes(ASTContextRef context);
void _ASTContextInitBuiltinFunctions(ASTContextRef context);

ASTBuiltinTypeRef _ASTContextGetBuiltinTypeByName(ASTContextRef context, const Char *name);

Bool _ASTArrayIsScopeLocationOrderedAscending(const void *lhs, const void *rhs);

ASTContextRef ASTContextCreate(AllocatorRef allocator, StringRef moduleName) {
    AllocatorRef bumpAllocator = BumpAllocatorCreate(allocator);
    ASTContextRef context      = AllocatorAllocate(bumpAllocator, sizeof(struct _ASTContext));
    context->allocator         = bumpAllocator;
    // TODO: @Bug Reallocation of dynamic arrays causes invalidation of all pointers do not store the source of truth in arrays!
    //            We can just allocate nodes dynamically without holding a reference to them because the BumpAllocator will be freed once...
    context->nodes[ASTTagSourceUnit]             = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTSourceUnit), 1024);
    context->nodes[ASTTagLinkedList]             = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTLinkedList), 1024);
    context->nodes[ASTTagArray]                  = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTArray), 2048);
    context->nodes[ASTTagLoadDirective]          = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTLoadDirective), 1024);
    context->nodes[ASTTagLinkDirective]          = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTLinkDirective), 1024);
    context->nodes[ASTTagBlock]                  = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTBlock), 1024);
    context->nodes[ASTTagIfStatement]            = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTIfStatement), 1024);
    context->nodes[ASTTagLoopStatement]          = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTLoopStatement), 1024);
    context->nodes[ASTTagCaseStatement]          = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTCaseStatement), 1024);
    context->nodes[ASTTagSwitchStatement]        = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTSwitchStatement), 1024);
    context->nodes[ASTTagControlStatement]       = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTControlStatement), 1024);
    context->nodes[ASTTagReferenceExpression]    = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTReferenceExpression), 1024);
    context->nodes[ASTTagDereferenceExpression]  = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTDereferenceExpression), 1024);
    context->nodes[ASTTagUnaryExpression]        = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTUnaryExpression), 1024);
    context->nodes[ASTTagBinaryExpression]       = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTBinaryExpression), 1024);
    context->nodes[ASTTagIdentifierExpression]   = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTIdentifierExpression), 1024);
    context->nodes[ASTTagMemberAccessExpression] = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTMemberAccessExpression), 1024);
    context->nodes[ASTTagAssignmentExpression]   = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTAssignmentExpression), 1024);
    context->nodes[ASTTagCallExpression]         = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTCallExpression), 1024);
    context->nodes[ASTTagConstantExpression]     = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTConstantExpression), 1024);
    context->nodes[ASTTagModuleDeclaration]      = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTModuleDeclaration), 1024);
    context->nodes[ASTTagEnumerationDeclaration] = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTEnumerationDeclaration), 1024);
    context->nodes[ASTTagFunctionDeclaration]    = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTFunctionDeclaration), 1024);
    context->nodes[ASTTagForeignFunctionDeclaration]   = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTFunctionDeclaration), 1024);
    context->nodes[ASTTagIntrinsicFunctionDeclaration] = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTFunctionDeclaration), 1024);
    context->nodes[ASTTagStructureDeclaration] = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTStructureDeclaration), 1024);
    context->nodes[ASTTagValueDeclaration]     = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTValueDeclaration), 1024);
    context->nodes[ASTTagOpaqueType]           = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTOpaqueType), 1024);
    context->nodes[ASTTagPointerType]          = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTPointerType), 1024);
    context->nodes[ASTTagArrayType]            = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTArrayType), 1024);
    context->nodes[ASTTagBuiltinType]          = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTBuiltinType), 1024);
    context->nodes[ASTTagEnumerationType]      = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTEnumerationType), 1024);
    context->nodes[ASTTagFunctionType]         = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTFunctionType), 1024);
    context->nodes[ASTTagStructureType]        = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTStructureType), 1024);
    context->nodes[ASTTagScope]                = ArrayCreateEmpty(context->allocator, sizeof(struct _ASTScope), 1024);
    context->module = ASTContextCreateModuleDeclaration(context, SourceRangeNull(), NULL, moduleName, NULL, NULL);
    _ASTContextInitBuiltinTypes(context);
    _ASTContextInitBuiltinFunctions(context);
    return context;
}

void ASTContextDestroy(ASTContextRef context) {
    AllocatorDestroy(context->allocator);
}

ASTScopeRef ASTContextGetGlobalScope(ASTContextRef context) {
    return context->module->scope;
}

ASTModuleDeclarationRef ASTContextGetModule(ASTContextRef context) {
    return context->module;
}

void ASTModuleAddSourceUnit(ASTContextRef context, ASTModuleDeclarationRef module, ASTSourceUnitRef sourceUnit) {
    ASTArrayAppendElement(module->sourceUnits, sourceUnit);
}

ASTSourceUnitRef ASTContextCreateSourceUnit(ASTContextRef context, SourceRange location, ASTScopeRef scope, StringRef filePath,
                                            ArrayRef declarations) {
    assert(filePath);

    ASTSourceUnitRef node = (ASTSourceUnitRef)_ASTContextCreateNode(context, ASTTagSourceUnit, location, scope);
    node->filePath        = StringCreateCopy(context->allocator, filePath);
    node->declarations    = ASTContextCreateArray(context, location, scope);
    if (declarations) {
        ASTArrayAppendArray(node->declarations, declarations);
    }
    return node;
}

ASTLinkedListRef ASTContextCreateLinkedList(ASTContextRef context, SourceRange location, ASTScopeRef scope) {
    ASTLinkedListRef list = (ASTLinkedListRef)_ASTContextCreateNode(context, ASTTagLinkedList, location, scope);
    list->node            = NULL;
    list->next            = NULL;
    return list;
}

ASTArrayRef ASTContextCreateArray(ASTContextRef context, SourceRange location, ASTScopeRef scope) {
    ASTArrayRef array   = (ASTArrayRef)_ASTContextCreateNode(context, ASTTagArray, location, scope);
    array->context      = context;
    array->elementCount = 0;
    array->list         = NULL;
    return array;
}

ASTLoadDirectiveRef ASTContextCreateLoadDirective(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                  ASTConstantExpressionRef filePath) {
    assert(filePath && filePath->kind == ASTConstantKindString);

    ASTLoadDirectiveRef node = (ASTLoadDirectiveRef)_ASTContextCreateNode(context, ASTTagLoadDirective, location, scope);
    node->filePath           = filePath;
    return node;
}

ASTLinkDirectiveRef ASTContextCreateLinkDirective(ASTContextRef context, SourceRange location, ASTScopeRef scope, StringRef library) {
    assert(library);

    ASTLinkDirectiveRef node = (ASTLinkDirectiveRef)_ASTContextCreateNode(context, ASTTagLinkDirective, location, scope);
    node->library            = StringCreateCopy(context->allocator, library);
    return node;
}

ASTBlockRef ASTContextCreateBlock(ASTContextRef context, SourceRange location, ASTScopeRef scope, ArrayRef statements) {
    ASTBlockRef node = (ASTBlockRef)_ASTContextCreateNode(context, ASTTagBlock, location, scope);
    node->statements = ASTContextCreateArray(context, location, scope);
    if (statements) {
        ASTArrayAppendArray(node->statements, statements);
    }
    return node;
}

ASTIfStatementRef ASTContextCreateIfStatement(ASTContextRef context, SourceRange location, ASTScopeRef scope, ASTExpressionRef condition,
                                              ASTBlockRef thenBlock, ASTBlockRef elseBlock) {
    assert(condition && thenBlock && elseBlock);

    ASTIfStatementRef node = (ASTIfStatementRef)_ASTContextCreateNode(context, ASTTagIfStatement, location, scope);
    node->condition        = condition;
    node->thenBlock        = thenBlock;
    node->elseBlock        = elseBlock;
    return node;
}

ASTLoopStatementRef ASTContextCreateLoopStatement(ASTContextRef context, SourceRange location, ASTScopeRef scope, ASTLoopKind kind,
                                                  ASTExpressionRef condition, ASTBlockRef loopBlock) {
    assert(condition && loopBlock);

    ASTLoopStatementRef node = (ASTLoopStatementRef)_ASTContextCreateNode(context, ASTTagLoopStatement, location, scope);
    node->kind               = kind;
    node->condition          = condition;
    node->loopBlock          = loopBlock;
    node->irEntry            = NULL;
    node->irExit             = NULL;
    return node;
}

ASTCaseStatementRef ASTContextCreateCaseStatement(ASTContextRef context, SourceRange location, ASTScopeRef scope, ASTCaseKind kind,
                                                  ASTExpressionRef condition, ASTBlockRef body) {
    assert((kind == ASTCaseKindElse || condition) && body);

    ASTCaseStatementRef node = (ASTCaseStatementRef)_ASTContextCreateNode(context, ASTTagCaseStatement, location, scope);
    node->kind               = kind;
    node->condition          = condition;
    node->body               = body;
    node->enclosingSwitch    = NULL;
    node->comparator         = NULL;
    node->irNext             = NULL;
    return node;
}

ASTSwitchStatementRef ASTContextCreateSwitchStatement(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                      ASTExpressionRef argument, ArrayRef cases) {
    assert(argument);

    ASTSwitchStatementRef node = (ASTSwitchStatementRef)_ASTContextCreateNode(context, ASTTagSwitchStatement, location, scope);
    node->argument             = argument;
    node->cases                = ASTContextCreateArray(context, location, scope);
    node->irExit               = NULL;
    if (cases) {
        ASTArrayAppendArray(node->cases, cases);
    }
    return node;
}

ASTControlStatementRef ASTContextCreateControlStatement(ASTContextRef context, SourceRange location, ASTScopeRef scope, ASTControlKind kind,
                                                        ASTExpressionRef result) {
    ASTControlStatementRef node = (ASTControlStatementRef)_ASTContextCreateNode(context, ASTTagControlStatement, location, scope);
    node->kind                  = kind;
    node->result                = result;
    node->enclosingNode         = NULL;
    return node;
}

ASTReferenceExpressionRef ASTContextCreateReferenceExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                              ASTExpressionRef argument) {
    assert(argument);

    ASTReferenceExpressionRef node = (ASTReferenceExpressionRef)_ASTContextCreateNode(context, ASTTagReferenceExpression, location, scope);
    node->argument                 = argument;
    node->base.type                = NULL;
    node->base.expectedType        = NULL;
    return node;
}

ASTDereferenceExpressionRef ASTContextCreateDereferenceExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                                  ASTExpressionRef argument) {
    assert(argument);

    ASTDereferenceExpressionRef node = (ASTDereferenceExpressionRef)_ASTContextCreateNode(context, ASTTagDereferenceExpression, location,
                                                                                          scope);
    node->argument                   = argument;
    node->base.type                  = NULL;
    node->base.expectedType          = NULL;
    return node;
}

ASTUnaryExpressionRef ASTContextCreateUnaryExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope, ASTUnaryOperator op,
                                                      ASTExpressionRef arguments[1]) {
    assert(arguments[0]);

    ASTUnaryExpressionRef node = (ASTUnaryExpressionRef)_ASTContextCreateNode(context, ASTTagUnaryExpression, location, scope);
    node->op                   = op;
    node->arguments[0]         = arguments[0];
    node->opFunction           = NULL;
    node->base.type            = NULL;
    node->base.expectedType    = NULL;
    return node;
}

ASTBinaryExpressionRef ASTContextCreateBinaryExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                        ASTBinaryOperator op, ASTExpressionRef arguments[2]) {
    assert(arguments[0] && arguments[1]);

    ASTBinaryExpressionRef node = (ASTBinaryExpressionRef)_ASTContextCreateNode(context, ASTTagBinaryExpression, location, scope);
    node->op                    = op;
    node->arguments[0]          = arguments[0];
    node->arguments[1]          = arguments[1];
    node->opFunction            = NULL;
    node->base.type             = NULL;
    node->base.expectedType     = NULL;
    return node;
}

ASTIdentifierExpressionRef ASTContextCreateIdentifierExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                                StringRef name) {
    assert(name);

    ASTIdentifierExpressionRef node = (ASTIdentifierExpressionRef)_ASTContextCreateNode(context, ASTTagIdentifierExpression, location,
                                                                                        scope);
    node->name                      = StringCreateCopy(context->allocator, name);
    node->base.type                 = NULL;
    node->base.expectedType         = NULL;
    node->candidateDeclarations     = ASTContextCreateArray(context, location, scope);
    node->resolvedDeclaration       = NULL;
    return node;
}

ASTMemberAccessExpressionRef ASTContextCreateMemberAccessExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                                    ASTExpressionRef argument, StringRef memberName) {
    assert(argument && memberName);

    ASTMemberAccessExpressionRef node = (ASTMemberAccessExpressionRef)_ASTContextCreateNode(context, ASTTagMemberAccessExpression, location,
                                                                                            scope);
    node->argument                    = argument;
    node->memberName                  = StringCreateCopy(context->allocator, memberName);
    node->memberIndex                 = -1;
    node->pointerDepth                = 0;
    node->base.type                   = NULL;
    node->base.expectedType           = NULL;
    return node;
}

ASTAssignmentExpressionRef ASTContextCreateAssignmentExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                                ASTBinaryOperator op, ASTExpressionRef variable,
                                                                ASTExpressionRef expression) {
    assert(variable && expression);

    ASTAssignmentExpressionRef node = (ASTAssignmentExpressionRef)_ASTContextCreateNode(context, ASTTagAssignmentExpression, location,
                                                                                        scope);
    node->op                        = op;
    node->variable                  = variable;
    node->expression                = expression;
    return node;
}

ASTCallExpressionRef ASTContextCreateCallExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope, ASTExpressionRef callee,
                                                    ArrayRef arguments) {
    assert(callee);

    ASTCallExpressionRef node = (ASTCallExpressionRef)_ASTContextCreateNode(context, ASTTagCallExpression, location, scope);
    node->callee              = callee;
    node->arguments           = ASTContextCreateArray(context, location, scope);
    node->base.type           = NULL;
    node->base.expectedType   = NULL;
    if (arguments) {
        ASTArrayAppendArray(node->arguments, arguments);
    }
    return node;
}

ASTConstantExpressionRef ASTContextCreateConstantNilExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope) {
    ASTConstantExpressionRef node = (ASTConstantExpressionRef)_ASTContextCreateNode(context, ASTTagConstantExpression, location, scope);
    node->kind                    = ASTConstantKindNil;
    node->minimumBitWidth         = -1;
    node->base.type               = NULL;
    node->base.expectedType       = NULL;
    return node;
}

ASTConstantExpressionRef ASTContextCreateConstantBoolExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                                Bool value) {
    ASTConstantExpressionRef node = (ASTConstantExpressionRef)_ASTContextCreateNode(context, ASTTagConstantExpression, location, scope);
    node->kind                    = ASTConstantKindBool;
    node->minimumBitWidth         = 1;
    node->boolValue               = value;
    node->base.type               = NULL;
    node->base.expectedType       = NULL;
    return node;
}

ASTConstantExpressionRef ASTContextCreateConstantIntExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                               UInt64 value) {
    ASTConstantExpressionRef node = (ASTConstantExpressionRef)_ASTContextCreateNode(context, ASTTagConstantExpression, location, scope);
    node->kind                    = ASTConstantKindInt;
    node->intValue                = value;
    node->base.type               = NULL;
    node->base.expectedType       = NULL;

    node->minimumBitWidth = 0;
    UInt64 intValue       = value;
    while (intValue > 0) {
        node->minimumBitWidth += 1;
        intValue >>= 1;
    }
    node->minimumBitWidth = MAX(node->minimumBitWidth, 1);

    return node;
}

ASTConstantExpressionRef ASTContextCreateConstantFloatExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                                 Float64 value) {
    ASTConstantExpressionRef node = (ASTConstantExpressionRef)_ASTContextCreateNode(context, ASTTagConstantExpression, location, scope);
    node->kind                    = ASTConstantKindFloat;
    node->minimumBitWidth         = -1;
    node->floatValue              = value;
    node->base.type               = NULL;
    node->base.expectedType       = NULL;
    return node;
}

ASTConstantExpressionRef ASTContextCreateConstantStringExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                                  StringRef value) {
    assert(value);

    ASTConstantExpressionRef node = (ASTConstantExpressionRef)_ASTContextCreateNode(context, ASTTagConstantExpression, location, scope);
    node->kind                    = ASTConstantKindString;
    node->stringValue             = StringCreateCopy(context->allocator, value);
    node->base.type               = NULL;
    node->base.expectedType       = NULL;
    return node;
}

ASTModuleDeclarationRef ASTContextCreateModuleDeclaration(ASTContextRef context, SourceRange location, ASTScopeRef scope, StringRef name,
                                                          ArrayRef sourceUnits, ArrayRef importedModules) {
    ASTModuleDeclarationRef node = (ASTModuleDeclarationRef)_ASTContextCreateNode(context, ASTTagModuleDeclaration, location, scope);
    node->base.name              = StringCreateCopy(context->allocator, name);
    node->base.mangledName       = NULL;
    node->base.type              = NULL;
    node->scope                  = ASTContextCreateScope(context, location, (ASTNodeRef)node, scope, ASTScopeKindGlobal);
    node->sourceUnits            = ASTContextCreateArray(context, location, scope);
    node->importedModules        = ASTContextCreateArray(context, location, scope);
    node->linkDirectives         = ASTContextCreateArray(context, location, scope);
    node->entryPointName         = StringCreate(context->allocator, "main");
    node->entryPoint             = NULL;
    if (sourceUnits) {
        ASTArrayAppendArray(node->sourceUnits, sourceUnits);
    }
    if (importedModules) {
        ASTArrayAppendArray(node->importedModules, importedModules);
    }
    return node;
}

ASTEnumerationDeclarationRef ASTContextCreateEnumerationDeclaration(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                                    StringRef name, ArrayRef elements) {
    assert(name);

    ASTEnumerationDeclarationRef node = (ASTEnumerationDeclarationRef)_ASTContextCreateNode(context, ASTTagEnumerationDeclaration, location,
                                                                                            scope);
    node->base.name                   = StringCreateCopy(context->allocator, name);
    node->base.mangledName            = NULL;
    node->elements                    = ASTContextCreateArray(context, location, scope);
    node->innerScope                  = NULL;
    if (elements) {
        ASTArrayAppendArray(node->elements, elements);
    }
    node->base.type = (ASTTypeRef)ASTContextCreateEnumerationType(context, location, scope, node);
    return node;
}

ASTFunctionDeclarationRef ASTContextCreateFunctionDeclaration(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                              ASTFixity fixity, StringRef name, ArrayRef parameters, ASTTypeRef returnType,
                                                              ASTBlockRef body) {
    assert(name && returnType && body);

    ASTFunctionDeclarationRef node = (ASTFunctionDeclarationRef)_ASTContextCreateNode(context, ASTTagFunctionDeclaration, location, scope);
    node->base.name                = StringCreateCopy(context->allocator, name);
    node->base.mangledName         = NULL;
    node->fixity                   = ASTFixityNone;
    node->parameters               = ASTContextCreateArray(context, location, scope);
    node->returnType               = returnType;
    node->body                     = body;
    node->innerScope               = NULL;
    if (parameters) {
        ASTArrayAppendArray(node->parameters, parameters);
    }
    node->base.type     = (ASTTypeRef)ASTContextCreateFunctionTypeForDeclaration(context, location, scope, node);
    node->foreignName   = NULL;
    node->intrinsicName = NULL;
    return node;
}

ASTFunctionDeclarationRef ASTContextCreateForeignFunctionDeclaration(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                                     ASTFixity fixity, StringRef name, ArrayRef parameters,
                                                                     ASTTypeRef returnType, StringRef foreignName) {
    assert(name && returnType && foreignName);

    ASTFunctionDeclarationRef node = (ASTFunctionDeclarationRef)_ASTContextCreateNode(context, ASTTagForeignFunctionDeclaration, location,
                                                                                      scope);
    node->base.name                = StringCreateCopy(context->allocator, name);
    node->base.mangledName         = NULL;
    node->fixity                   = fixity;
    node->parameters               = ASTContextCreateArray(context, location, scope);
    node->returnType               = returnType;
    node->body                     = NULL;
    if (parameters) {
        ASTArrayAppendArray(node->parameters, parameters);
    }
    node->base.type     = (ASTTypeRef)ASTContextCreateFunctionTypeForDeclaration(context, location, scope, node);
    node->foreignName   = StringCreateCopy(context->allocator, foreignName);
    node->intrinsicName = NULL;
    return node;
}

ASTFunctionDeclarationRef ASTContextCreateIntrinsicFunctionDeclaration(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                                       ASTFixity fixity, StringRef name, ArrayRef parameters,
                                                                       ASTTypeRef returnType, StringRef intrinsicName) {
    assert(name && returnType && intrinsicName);

    ASTFunctionDeclarationRef node = (ASTFunctionDeclarationRef)_ASTContextCreateNode(context, ASTTagIntrinsicFunctionDeclaration, location,
                                                                                      scope);
    node->base.name                = StringCreateCopy(context->allocator, name);
    node->base.mangledName         = NULL;
    node->fixity                   = fixity;
    node->parameters               = ASTContextCreateArray(context, location, scope);
    node->returnType               = returnType;
    node->body                     = NULL;
    if (parameters) {
        ASTArrayAppendArray(node->parameters, parameters);
    }
    node->base.type     = (ASTTypeRef)ASTContextCreateFunctionTypeForDeclaration(context, location, scope, node);
    node->foreignName   = NULL;
    node->intrinsicName = StringCreateCopy(context->allocator, intrinsicName);
    return node;
}

ASTStructureDeclarationRef ASTContextCreateStructureDeclaration(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                                StringRef name, ArrayRef values) {
    assert(name);

    ASTStructureDeclarationRef node = (ASTStructureDeclarationRef)_ASTContextCreateNode(context, ASTTagStructureDeclaration, location,
                                                                                        scope);
    node->base.name                 = StringCreateCopy(context->allocator, name);
    node->base.mangledName          = NULL;
    node->values                    = ASTContextCreateArray(context, location, scope);
    node->innerScope                = NULL;
    if (values) {
        ASTArrayAppendArray(node->values, values);
    }
    node->base.type = (ASTTypeRef)ASTContextCreateStructureType(context, location, scope, node);
    return node;
}

ASTValueDeclarationRef ASTContextCreateValueDeclaration(ASTContextRef context, SourceRange location, ASTScopeRef scope, ASTValueKind kind,
                                                        StringRef name, ASTTypeRef type, ASTExpressionRef initializer) {
    assert(name && type);
    assert((kind == ASTValueKindParameter && !initializer) || (kind == ASTValueKindVariable || kind == ASTValueKindEnumerationElement));

    ASTValueDeclarationRef node = (ASTValueDeclarationRef)_ASTContextCreateNode(context, ASTTagValueDeclaration, location, scope);
    node->base.name             = StringCreateCopy(context->allocator, name);
    node->base.mangledName      = NULL;
    node->kind                  = kind;
    node->base.type             = type;
    node->initializer           = initializer;
    return node;
}

ASTOpaqueTypeRef ASTContextCreateOpaqueType(ASTContextRef context, SourceRange location, ASTScopeRef scope, StringRef name) {
    assert(name);

    ASTOpaqueTypeRef node = (ASTOpaqueTypeRef)_ASTContextCreateNode(context, ASTTagOpaqueType, location, scope);
    node->name            = StringCreateCopy(context->allocator, name);
    node->declaration     = NULL;
    return node;
}

ASTPointerTypeRef ASTContextCreatePointerType(ASTContextRef context, SourceRange location, ASTScopeRef scope, ASTTypeRef pointeeType) {
    assert(pointeeType);

    ASTPointerTypeRef node = (ASTPointerTypeRef)_ASTContextCreateNode(context, ASTTagPointerType, location, scope);
    node->pointeeType      = pointeeType;
    return node;
}

ASTArrayTypeRef ASTContextCreateArrayType(ASTContextRef context, SourceRange location, ASTScopeRef scope, ASTTypeRef elementType,
                                          ASTExpressionRef size) {
    assert(elementType);

    ASTArrayTypeRef node = (ASTArrayTypeRef)_ASTContextCreateNode(context, ASTTagArrayType, location, scope);
    node->elementType    = elementType;
    node->size           = size;
    return node;
}

ASTEnumerationTypeRef ASTContextCreateEnumerationType(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                      ASTEnumerationDeclarationRef declaration) {
    ASTEnumerationTypeRef node = (ASTEnumerationTypeRef)_ASTContextCreateNode(context, ASTTagEnumerationType, location, scope);
    node->declaration          = declaration;
    return node;
}

ASTFunctionTypeRef ASTContextCreateFunctionTypeForDeclaration(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                              ASTFunctionDeclarationRef declaration) {
    ASTFunctionTypeRef node = (ASTFunctionTypeRef)_ASTContextCreateNode(context, ASTTagFunctionType, location, scope);
    node->declaration       = declaration;
    node->parameterTypes    = ASTContextCreateArray(context, location, scope);
    node->resultType        = declaration->returnType;

    ASTArrayIteratorRef iterator = ASTArrayGetIterator(declaration->parameters);
    while (iterator) {
        ASTValueDeclarationRef parameter = (ASTValueDeclarationRef)ASTArrayIteratorGetElement(iterator);
        ASTArrayAppendElement(node->parameterTypes, parameter->base.type);

        iterator = ASTArrayIteratorNext(iterator);
    }

    return node;
}

ASTFunctionTypeRef ASTContextCreateFunctionType(ASTContextRef context, SourceRange location, ASTScopeRef scope, ArrayRef parameterTypes,
                                                ASTTypeRef resultType) {
    assert(resultType);

    ASTFunctionTypeRef node = (ASTFunctionTypeRef)_ASTContextCreateNode(context, ASTTagFunctionType, location, scope);
    node->declaration       = NULL;
    node->parameterTypes    = ASTContextCreateArray(context, location, scope);
    node->resultType        = resultType;
    if (parameterTypes) {
        ASTArrayAppendArray(node->parameterTypes, parameterTypes);
    }
    return node;
}

ASTStructureTypeRef ASTContextCreateStructureType(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                  ASTStructureDeclarationRef declaration) {
    ASTStructureTypeRef node = (ASTStructureTypeRef)_ASTContextCreateNode(context, ASTTagStructureType, location, scope);
    node->declaration        = declaration;
    return node;
}

ASTScopeRef ASTContextCreateScope(ASTContextRef context, SourceRange location, ASTNodeRef node, ASTScopeRef parent, ASTScopeKind kind) {
    ASTScopeRef scope   = (ASTScopeRef)_ASTContextCreateNode(context, ASTTagScope, location, parent);
    scope->node         = node;
    scope->kind         = kind;
    scope->parent       = parent;
    scope->children     = ASTContextCreateArray(context, location, scope);
    scope->declarations = ASTContextCreateArray(context, location, scope);
    scope->context      = context;

    if (parent) {
        //        assert(parent->base.location.start <= location.start && location.end <= parent->base.location.end);

        Index index = ASTArrayGetSortedInsertionIndex(parent->children, &_ASTArrayIsScopeLocationOrderedAscending, scope);
        ASTArrayInsertElementAtIndex(parent->children, index, scope);
    }
    return scope;
}

ASTBuiltinTypeRef ASTContextGetBuiltinType(ASTContextRef context, ASTBuiltinTypeKind kind) {
    return context->builtinTypes[kind];
}

ASTStructureTypeRef ASTContextGetStringType(ASTContextRef context) {
    return context->stringType;
}

ASTNodeRef _ASTContextCreateNode(ASTContextRef context, ASTTag tag, SourceRange location, ASTScopeRef scope) {
    ASTNodeRef node = ArrayAppendUninitializedElement(context->nodes[tag]);
    node->tag       = tag;
    node->flags     = ASTFlagsNone;
    node->location  = location;
    node->scope     = scope;
    node->irValue   = NULL;
    node->irType    = NULL;
    return node;
}

ASTBuiltinTypeRef _ASTContextCreateBuiltinType(ASTContextRef context, SourceRange location, ASTScopeRef scope, ASTBuiltinTypeKind kind) {
    ASTBuiltinTypeRef node = (ASTBuiltinTypeRef)_ASTContextCreateNode(context, ASTTagBuiltinType, location, scope);
    node->kind             = kind;
    return node;
}

// TODO: Move builtin types to a builtin module and implicitly import the module to the main module
void _ASTContextInitBuiltinTypes(ASTContextRef context) {
    ASTScopeRef globalScope = ASTContextGetGlobalScope(context);

    const Char *builtinTypeNames[AST_BUILTIN_TYPE_KIND_COUNT] = {
        "<error>", "Void",   "Bool",   "Int8",   "Int16", "Int32",   "Int64",   "Int",
        "UInt8",   "UInt16", "UInt32", "UInt64", "UInt",  "Float32", "Float64", "Float",
    };

    StringRef name                                 = StringCreate(context->allocator, builtinTypeNames[ASTBuiltinTypeKindError]);
    context->builtinTypes[ASTBuiltinTypeKindError] = _ASTContextCreateBuiltinType(context, SourceRangeNull(), globalScope,
                                                                                  ASTBuiltinTypeKindError);

    // NOTE: Iteration begins after ASTBuiltinTypeKindError which is 0 to skip addition of <error> type to the scope.
    for (Index index = ASTBuiltinTypeKindError + 1; index < AST_BUILTIN_TYPE_KIND_COUNT; index++) {
        name                         = StringCreate(context->allocator, builtinTypeNames[index]);
        context->builtinTypes[index] = _ASTContextCreateBuiltinType(context, SourceRangeNull(), globalScope, (ASTBuiltinTypeKind)index);
        // TODO: May replace structure declaration with some builtin declaration?
        ASTStructureDeclarationRef structure = ASTContextCreateStructureDeclaration(context, SourceRangeNull(), globalScope, name, NULL);
        structure->innerScope                = ASTContextCreateScope(context, SourceRangeNull(), (ASTNodeRef)structure, globalScope,
                                                      ASTScopeKindStructure);
        structure->base.type                 = (ASTTypeRef)context->builtinTypes[index];
        ASTScopeInsertDeclaration(globalScope, (ASTDeclarationRef)structure);
    }

    StringRef stringName        = StringCreate(context->allocator, "String");
    ASTScopeRef stringScope     = ASTContextCreateScope(context, SourceRangeNull(), NULL, globalScope, ASTScopeKindStructure);
    ASTTypeRef stringBufferType = (ASTTypeRef)ASTContextCreatePointerType(
        context, SourceRangeNull(), stringScope, (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindUInt8));
    StringRef stringBufferName          = StringCreate(context->allocator, "buffer");
    ASTValueDeclarationRef stringBuffer = ASTContextCreateValueDeclaration(context, SourceRangeNull(), stringScope, ASTValueKindVariable,
                                                                           stringBufferName, stringBufferType, NULL);
    ASTTypeRef stringCountType          = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindInt);
    StringRef stringCountName           = StringCreate(context->allocator, "count");
    ASTValueDeclarationRef stringCount  = ASTContextCreateValueDeclaration(context, SourceRangeNull(), stringScope, ASTValueKindVariable,
                                                                          stringCountName, stringCountType, NULL);
    ArrayRef stringValues               = ArrayCreateEmpty(context->allocator, sizeof(ASTValueDeclarationRef), 2);
    ArrayAppendElement(stringValues, &stringBuffer);
    ArrayAppendElement(stringValues, &stringCount);
    ASTStructureDeclarationRef stringDeclaration = ASTContextCreateStructureDeclaration(context, SourceRangeNull(), globalScope, stringName,
                                                                                        stringValues);
    PerformNameManglingForDeclaration(context, (ASTDeclarationRef)stringDeclaration);
    context->stringType = ASTContextCreateStructureType(context, SourceRangeNull(), globalScope, stringDeclaration);
    ArrayDestroy(stringValues);
    StringDestroy(stringCountName);
    StringDestroy(stringBufferName);
    StringDestroy(stringName);
}

void _ASTContextInitBuiltinFunctions(ASTContextRef context) {
    ASTScopeRef globalScope = ASTContextGetGlobalScope(context);

#define UNARY_OPERATOR(SYMBOL, ARGUMENT_TYPE, RESULT_TYPE, INTRINSIC)                                                                      \
    {                                                                                                                                      \
        ArrayRef parameters              = ArrayCreateEmpty(AllocatorGetSystemDefault(), sizeof(ASTValueDeclarationRef), 1);               \
        ASTScopeRef scope                = ASTContextCreateScope(context, SourceRangeNull(), NULL, globalScope, ASTScopeKindFunction);     \
        StringRef parameterName          = StringCreate(context->allocator, "value");                                                      \
        ASTTypeRef parameterType         = (ASTTypeRef)_ASTContextGetBuiltinTypeByName(context, ARGUMENT_TYPE);                            \
        ASTValueDeclarationRef parameter = ASTContextCreateValueDeclaration(context, SourceRangeNull(), scope, ASTValueKindParameter,      \
                                                                            parameterName, parameterType, NULL);                           \
        ArrayAppendElement(parameters, &parameter);                                                                                        \
        ASTTypeRef resultType                 = (ASTTypeRef)_ASTContextGetBuiltinTypeByName(context, RESULT_TYPE);                         \
        StringRef symbol                      = StringCreate(context->allocator, SYMBOL);                                                  \
        StringRef intrinsic                   = StringCreate(context->allocator, INTRINSIC);                                               \
        ASTFunctionDeclarationRef declaration = ASTContextCreateIntrinsicFunctionDeclaration(                                              \
            context, SourceRangeNull(), globalScope, ASTFixityPrefix, symbol, parameters, resultType, intrinsic);                          \
        scope->node = (ASTNodeRef)declaration;                                                                                             \
        ASTScopeInsertDeclaration(globalScope, (ASTDeclarationRef)declaration);                                                            \
        ArrayDestroy(parameters);                                                                                                          \
    }

#define BINARY_OPERATOR(SYMBOL, ARGUMENT_TYPE1, ARGUMENT_TYPE2, RESULT_TYPE, INTRINSIC)                                                    \
    {                                                                                                                                      \
        ArrayRef parameters                 = ArrayCreateEmpty(AllocatorGetSystemDefault(), sizeof(ASTValueDeclarationRef), 2);            \
        ASTScopeRef scope                   = ASTContextCreateScope(context, SourceRangeNull(), NULL, globalScope, ASTScopeKindFunction);  \
        StringRef lhsParameterName          = StringCreate(context->allocator, "lhs");                                                     \
        ASTTypeRef lhsParameterType         = (ASTTypeRef)_ASTContextGetBuiltinTypeByName(context, ARGUMENT_TYPE1);                        \
        ASTValueDeclarationRef lhsParameter = ASTContextCreateValueDeclaration(context, SourceRangeNull(), scope, ASTValueKindParameter,   \
                                                                               lhsParameterName, lhsParameterType, NULL);                  \
        StringRef rhsParameterName          = StringCreate(context->allocator, "rhs");                                                     \
        ASTTypeRef rhsParameterType         = (ASTTypeRef)_ASTContextGetBuiltinTypeByName(context, ARGUMENT_TYPE2);                        \
        ASTValueDeclarationRef rhsParameter = ASTContextCreateValueDeclaration(context, SourceRangeNull(), scope, ASTValueKindParameter,   \
                                                                               rhsParameterName, rhsParameterType, NULL);                  \
        ArrayAppendElement(parameters, &lhsParameter);                                                                                     \
        ArrayAppendElement(parameters, &rhsParameter);                                                                                     \
        ASTTypeRef resultType                 = (ASTTypeRef)_ASTContextGetBuiltinTypeByName(context, RESULT_TYPE);                         \
        StringRef intrinsic                   = StringCreate(context->allocator, INTRINSIC);                                               \
        StringRef symbol                      = StringCreate(context->allocator, SYMBOL);                                                  \
        ASTFunctionDeclarationRef declaration = ASTContextCreateIntrinsicFunctionDeclaration(                                              \
            context, SourceRangeNull(), globalScope, ASTFixityInfix, symbol, parameters, resultType, intrinsic);                           \
        scope->node = (ASTNodeRef)declaration;                                                                                             \
        ASTScopeInsertDeclaration(globalScope, (ASTDeclarationRef)declaration);                                                            \
        ArrayDestroy(parameters);                                                                                                          \
    }

#include "JellyCore/RuntimeSupportDefinitions.h"
}

ASTBuiltinTypeRef _ASTContextGetBuiltinTypeByName(ASTContextRef context, const Char *name) {
    const Char *builtinTypeNames[AST_BUILTIN_TYPE_KIND_COUNT - 1] = {
        "Void",   "Bool",   "Int8",   "Int16", "Int32",   "Int64",   "Int",   "UInt8",
        "UInt16", "UInt32", "UInt64", "UInt",  "Float32", "Float64", "Float",
    };

    for (Index index = 0; index < AST_BUILTIN_TYPE_KIND_COUNT - 1; index++) {
        if (strcmp(builtinTypeNames[index], name) == 0) {
            return ASTContextGetBuiltinType(context, (ASTBuiltinTypeKind)(index + 1));
        }
    }

    return NULL;
}

Bool _ASTArrayIsScopeLocationOrderedAscending(const void *lhs, const void *rhs) {
    ASTScopeRef lhsScope = (ASTScopeRef)lhs;
    ASTScopeRef rhsScope = (ASTScopeRef)rhs;

    if (lhsScope->base.location.start == rhsScope->base.location.start) {
        return lhsScope->base.location.end < rhsScope->base.location.end;
    }

    return lhsScope->base.location.start < rhsScope->base.location.start;
}
