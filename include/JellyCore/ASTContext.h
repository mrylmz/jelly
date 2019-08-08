#ifndef __JELLY_ASTCONTEXT__
#define __JELLY_ASTCONTEXT__

#include <JellyCore/ASTNodes.h>
#include <JellyCore/Allocator.h>
#include <JellyCore/Base.h>

JELLY_EXTERN_C_BEGIN

typedef struct _ASTContext *ASTContextRef;

ASTContextRef ASTContextCreate(AllocatorRef allocator, StringRef moduleName);

void ASTContextDestroy(ASTContextRef context);

ASTScopeRef ASTContextGetGlobalScope(ASTContextRef context);

ASTModuleDeclarationRef ASTContextGetModule(ASTContextRef context);

ArrayRef ASTContextGetAllNodes(ASTContextRef context, ASTTag tag);

void ASTModuleAddSourceUnit(ASTContextRef context, ASTModuleDeclarationRef module, ASTSourceUnitRef sourceUnit);

ASTSourceUnitRef ASTContextCreateSourceUnit(ASTContextRef context, SourceRange location, ASTScopeRef scope, StringRef filePath,
                                            ArrayRef declarations);

ASTLinkedListRef ASTContextCreateLinkedList(ASTContextRef context, SourceRange location, ASTScopeRef scope);

ASTArrayRef ASTContextCreateArray(ASTContextRef context, SourceRange location, ASTScopeRef scope);

ASTLoadDirectiveRef ASTContextCreateLoadDirective(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                  ASTConstantExpressionRef filePath);

ASTLinkDirectiveRef ASTContextCreateLinkDirective(ASTContextRef context, SourceRange location, ASTScopeRef scope, StringRef library);

ASTBlockRef ASTContextCreateBlock(ASTContextRef context, SourceRange location, ASTScopeRef scope, ArrayRef statements);

ASTIfStatementRef ASTContextCreateIfStatement(ASTContextRef context, SourceRange location, ASTScopeRef scope, ASTExpressionRef condition,
                                              ASTBlockRef thenBlock, ASTBlockRef elseBlock);
ASTLoopStatementRef ASTContextCreateLoopStatement(ASTContextRef context, SourceRange location, ASTScopeRef scope, ASTLoopKind kind,
                                                  ASTExpressionRef condition, ASTBlockRef loopBlock);
ASTCaseStatementRef ASTContextCreateCaseStatement(ASTContextRef context, SourceRange location, ASTScopeRef scope, ASTCaseKind kind,
                                                  ASTExpressionRef condition, ASTBlockRef body);
ASTSwitchStatementRef ASTContextCreateSwitchStatement(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                      ASTExpressionRef argument, ArrayRef cases);
ASTControlStatementRef ASTContextCreateControlStatement(ASTContextRef context, SourceRange location, ASTScopeRef scope, ASTControlKind kind,
                                                        ASTExpressionRef result);
ASTReferenceExpressionRef ASTContextCreateReferenceExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                              ASTExpressionRef argument);
ASTDereferenceExpressionRef ASTContextCreateDereferenceExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                                  ASTExpressionRef argument);
ASTUnaryExpressionRef ASTContextCreateUnaryExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope, ASTUnaryOperator op,
                                                      ASTExpressionRef arguments[1]);
ASTBinaryExpressionRef ASTContextCreateBinaryExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                        ASTBinaryOperator op, ASTExpressionRef arguments[2]);
ASTIdentifierExpressionRef ASTContextCreateIdentifierExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                                StringRef name);
ASTMemberAccessExpressionRef ASTContextCreateMemberAccessExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                                    ASTExpressionRef argument, StringRef memberName);
ASTAssignmentExpressionRef ASTContextCreateAssignmentExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                                ASTBinaryOperator op, ASTExpressionRef variable,
                                                                ASTExpressionRef expression);
ASTCallExpressionRef ASTContextCreateCallExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope, ASTExpressionRef callee,
                                                    ArrayRef arguments);
ASTCallExpressionRef ASTContextCreateUnaryCallExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                         ASTUnaryOperator op, ASTExpressionRef arguments[1]);
ASTCallExpressionRef ASTContextCreateBinaryCallExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                          ASTBinaryOperator op, ASTExpressionRef arguments[2]);
ASTConstantExpressionRef ASTContextCreateConstantNilExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope);
ASTConstantExpressionRef ASTContextCreateConstantBoolExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope, Bool value);
ASTConstantExpressionRef ASTContextCreateConstantIntExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                               UInt64 value);
ASTConstantExpressionRef ASTContextCreateConstantFloatExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                                 Float64 value);
ASTConstantExpressionRef ASTContextCreateConstantStringExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                                  StringRef value);

ASTSizeOfExpressionRef ASTContextCreateSizeOfExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                        ASTTypeRef sizeType);

ASTTypeOperationExpressionRef ASTContextCreateTypeOperationExpression(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                                      ASTTypeOperation op, ASTExpressionRef expression,
                                                                      ASTTypeRef expressionType);

ASTModuleDeclarationRef ASTContextCreateModuleDeclaration(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                          StringRef moduleName, ArrayRef sourceUnits, ArrayRef importedModules);

ASTEnumerationDeclarationRef ASTContextCreateEnumerationDeclaration(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                                    StringRef name, ArrayRef elements);

ASTFunctionDeclarationRef ASTContextCreateFunctionDeclaration(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                              ASTFixity fixity, StringRef name, ArrayRef parameters, ASTTypeRef returnType,
                                                              ASTBlockRef body);

ASTFunctionDeclarationRef ASTContextCreateForeignFunctionDeclaration(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                                     ASTFixity fixity, StringRef name, ArrayRef parameters,
                                                                     ASTTypeRef returnType, StringRef foreignName);

ASTFunctionDeclarationRef ASTContextCreateIntrinsicFunctionDeclaration(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                                       ASTFixity fixity, StringRef name, ArrayRef parameters,
                                                                       ASTTypeRef returnType, StringRef intrinsicName);

ASTStructureDeclarationRef ASTContextCreateStructureDeclaration(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                                StringRef name, ArrayRef values, ArrayRef initializers);

ASTInitializerDeclarationRef ASTContextCreateInitializerDeclaration(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                                    ArrayRef parameters, ASTBlockRef body);

ASTValueDeclarationRef ASTContextCreateValueDeclaration(ASTContextRef context, SourceRange location, ASTScopeRef scope, ASTValueKind kind,
                                                        StringRef name, ASTTypeRef type, ASTExpressionRef initializer);

ASTTypeAliasDeclarationRef ASTContextCreateTypeAliasDeclaration(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                                StringRef name, ASTTypeRef type);

ASTOpaqueTypeRef ASTContextCreateOpaqueType(ASTContextRef context, SourceRange location, ASTScopeRef scope, StringRef name);

ASTPointerTypeRef ASTContextCreatePointerType(ASTContextRef context, SourceRange location, ASTScopeRef scope, ASTTypeRef pointeeType);

ASTArrayTypeRef ASTContextCreateArrayType(ASTContextRef context, SourceRange location, ASTScopeRef scope, ASTTypeRef elementType,
                                          ASTExpressionRef size);

ASTEnumerationTypeRef ASTContextCreateEnumerationType(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                      ASTEnumerationDeclarationRef declaration);

ASTFunctionTypeRef ASTContextCreateFunctionTypeForDeclaration(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                              ASTFunctionDeclarationRef declaration);

ASTFunctionTypeRef ASTContextCreateFunctionType(ASTContextRef context, SourceRange location, ASTScopeRef scope, ArrayRef parameterTypes,
                                                ASTTypeRef resultType);

ASTStructureTypeRef ASTContextCreateStructureType(ASTContextRef context, SourceRange location, ASTScopeRef scope,
                                                  ASTStructureDeclarationRef declaration);

ASTScopeRef ASTContextCreateScope(ASTContextRef context, SourceRange location, ASTNodeRef node, ASTScopeRef parent, ASTScopeKind kind);

ASTBuiltinTypeRef ASTContextGetBuiltinType(ASTContextRef context, ASTBuiltinTypeKind kind);

ASTStructureTypeRef ASTContextGetStringType(ASTContextRef context);

JELLY_EXTERN_C_END

#endif
