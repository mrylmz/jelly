#ifndef __JELLY_ASTARRAY__
#define __JELLY_ASTARRAY__

#include <JellyCore/Array.h>
#include <JellyCore/Base.h>

JELLY_EXTERN_C_BEGIN

typedef struct _ASTArray *ASTArrayRef;

typedef Bool (*ASTArrayPredicate)(const void *elementLeft, const void *elementRight);

Index ASTArrayGetElementCount(ASTArrayRef array);

void *ASTArrayGetElementAtIndex(ASTArrayRef array, Index index);

void ASTArrayAppendElement(ASTArrayRef array, void *element);

void ASTArrayAppendASTArray(ASTArrayRef array, ASTArrayRef other);

void ASTArrayAppendArray(ASTArrayRef array, ArrayRef other);

void ASTArrayInsertElementAtIndex(ASTArrayRef array, Index index, void *element);

void ASTArraySetElementAtIndex(ASTArrayRef array, Index index, void *element);

void ASTArrayRemoveElementAtIndex(ASTArrayRef array, Index index);

void ASTArrayRemoveAllElements(ASTArrayRef array);

bool ASTArrayContainsElement(ASTArrayRef array, ASTArrayPredicate predicate, void *element);

bool ASTArrayIsEqual(ASTArrayRef lhs, ASTArrayRef rhs);

JELLY_EXTERN_C_END

#endif
