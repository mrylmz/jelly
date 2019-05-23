#ifndef __JELLY_ASTCONTEXT__
#define __JELLY_ASTCONTEXT__

#include <JellyCore/ASTNodes.h>
#include <JellyCore/Allocator.h>
#include <JellyCore/Base.h>

JELLY_EXTERN_C_BEGIN

typedef struct _ASTContext *ASTContextRef;

ASTContextRef ASTContextCreate(AllocatorRef allocator);

void ASTContextDestroy(ASTContextRef context);

ASTSourceUnitRef ASTContextCreateSourceUnit(ASTContextRef context, SourceRange location, StringRef filePath, ArrayRef declarations);

ASTLoadDirectiveRef ASTContextCreateLoadDirective(ASTContextRef context, SourceRange location, StringRef filePath);

ASTBlockRef ASTContextCreateBlock(ASTContextRef context, SourceRange location, ArrayRef statements);

ASTIfStatementRef ASTContextCreateIfStatement(ASTContextRef context, SourceRange location, ASTExpressionRef condition,
                                              ASTBlockRef thenBlock, ASTBlockRef elseBlock);
ASTLoopStatementRef ASTContextCreateLoopStatement(ASTContextRef context, SourceRange location, ASTLoopKind kind, ASTExpressionRef condition,
                                                  ASTBlockRef loopBlock);
ASTCaseStatementRef ASTContextCreateCaseStatement(ASTContextRef context, SourceRange location, ASTCaseKind kind, ASTExpressionRef condition,
                                                  ASTBlockRef body);
ASTSwitchStatementRef ASTContextCreateSwitchStatement(ASTContextRef context, SourceRange location, ASTExpressionRef argument,
                                                      ArrayRef cases);
ASTControlStatementRef ASTContextCreateControlStatement(ASTContextRef context, SourceRange location, ASTControlKind kind,
                                                        ASTExpressionRef result);
ASTUnaryExpressionRef ASTContextCreateUnaryExpression(ASTContextRef context, SourceRange location, ASTUnaryOperator op,
                                                      ASTExpressionRef arguments[1]);
ASTBinaryExpressionRef ASTContextCreateBinaryExpression(ASTContextRef context, SourceRange location, ASTBinaryOperator op,
                                                        ASTExpressionRef arguments[2]);
ASTIdentifierExpressionRef ASTContextCreateIdentifierExpression(ASTContextRef context, SourceRange location, StringRef name);
ASTMemberAccessExpressionRef ASTContextCreateMemberAccessExpression(ASTContextRef context, SourceRange location, ASTExpressionRef argument,
                                                                    StringRef memberName);
ASTCallExpressionRef ASTContextCreateCallExpression(ASTContextRef context, SourceRange location, ASTExpressionRef callee,
                                                    ArrayRef arguments);
ASTConstantExpressionRef ASTContextCreateConstantNilExpression(ASTContextRef context, SourceRange location);
ASTConstantExpressionRef ASTContextCreateConstantBoolExpression(ASTContextRef context, SourceRange location, Bool value);
ASTConstantExpressionRef ASTContextCreateConstantIntExpression(ASTContextRef context, SourceRange location, UInt64 value);
ASTConstantExpressionRef ASTContextCreateConstantFloatExpression(ASTContextRef context, SourceRange location, Float64 value);
ASTConstantExpressionRef ASTContextCreateConstantStringExpression(ASTContextRef context, SourceRange location, StringRef value);
ASTModuleDeclarationRef ASTContextCreateModuleDeclaration(ASTContextRef context, SourceRange location, ArrayRef sourceUnits,
                                                          ArrayRef importedModules);
ASTEnumerationDeclarationRef ASTContextCreateEnumerationDeclaration(ASTContextRef context, SourceRange location, StringRef name,
                                                                    ArrayRef elements);
ASTFunctionDeclarationRef ASTContextCreateFunctionDeclaration(ASTContextRef context, SourceRange location, StringRef name,
                                                              ArrayRef parameters, ASTTypeRef returnType, ASTBlockRef body);
ASTStructureDeclarationRef ASTContextCreateStructureDeclaration(ASTContextRef context, SourceRange location, StringRef name,
                                                                ArrayRef values);
ASTOpaqueDeclarationRef ASTContextCreateOpaqueDeclaration(ASTContextRef context, SourceRange location, StringRef name);
ASTValueDeclarationRef ASTContextCreateValueDeclaration(ASTContextRef context, SourceRange location, ASTValueKind kind, StringRef name,
                                                        ASTTypeRef type, ASTExpressionRef initializer);

ASTOpaqueTypeRef ASTContextCreateOpaqueType(ASTContextRef context, SourceRange location, StringRef name);

ASTPointerTypeRef ASTContextCreatePointerType(ASTContextRef context, SourceRange location, ASTTypeRef pointeeType, UInt64 depth);

ASTArrayTypeRef ASTContextCreateArrayType(ASTContextRef context, SourceRange location, ASTTypeRef elementType, ASTExpressionRef size);

JELLY_EXTERN_C_END

#endif
