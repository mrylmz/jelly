#include "JellyCore/ASTArray.h"
#include "JellyCore/ASTContext.h"

Index ASTArrayGetSortedInsertionIndex(ASTArrayRef array, ASTArrayPredicate isOrderedAscending, void *element) {
    if (ASTArrayGetElementCount(array) < 1) {
        return 0;
    }

    Index index = ASTArrayGetElementCount(array);
    while (0 < index && isOrderedAscending(element, ASTArrayGetElementAtIndex(array, index - 1))) {
        index -= 1;
    }

    return index;
}

Index ASTArrayGetElementCount(ASTArrayRef array) {
    return array->elementCount;
}

void *ASTArrayGetElementAtIndex(ASTArrayRef array, Index index) {
    assert(index < array->elementCount);

    ASTLinkedListRef list = array->list;
    for (Index offset = 0; offset < index; offset++) {
        assert(list->next);
        list = list->next;
    }

    return list->node;
}

void ASTArrayAppendElement(ASTArrayRef array, void *element) {
    if (!array->list) {
        array->list       = ASTContextCreateLinkedList(array->context, array->base.location, array->base.scope);
        array->list->node = element;
        array->elementCount += 1;
        return;
    }

    ASTLinkedListRef list = array->list;
    while (list && list->next) {
        list = list->next;
    }

    list->next       = ASTContextCreateLinkedList(array->context, array->base.location, array->base.scope);
    list->next->node = element;
    array->elementCount += 1;
}

void ASTArrayAppendASTArray(ASTArrayRef array, ASTArrayRef other) {
    for (Index index = 0; index < other->elementCount; index++) {
        ASTArrayAppendElement(array, ASTArrayGetElementAtIndex(other, index));
    }
}

void ASTArrayAppendArray(ASTArrayRef array, ArrayRef other) {
    for (Index index = 0; index < ArrayGetElementCount(other); index++) {
        void *element = *((void **)ArrayGetElementAtIndex(other, index));
        ASTArrayAppendElement(array, element);
    }
}

void ASTArrayInsertElementAtIndex(ASTArrayRef array, Index index, void *element) {
    if (index == array->elementCount) {
        return ASTArrayAppendElement(array, element);
    }

    ASTLinkedListRef list = array->list;
    if (index > 0) {
        for (Index offset = 0; offset < index - 1; offset++) {
            assert(list->next);
            list = list->next;
        }
    }

    ASTLinkedListRef next = list->next;
    list->next            = ASTContextCreateLinkedList(array->context, array->base.location, array->base.scope);
    list->next->node      = element;
    list->next->next      = next;
    array->elementCount += 1;
}

void ASTArraySetElementAtIndex(ASTArrayRef array, Index index, void *element) {
    assert(index < array->elementCount);

    ASTLinkedListRef list = array->list;
    for (Index offset = 0; offset < index; offset++) {
        assert(list->next);
        list = list->next;
    }

    list->node = element;
}

void ASTArrayRemoveElementAtIndex(ASTArrayRef array, Index index) {
    assert(index < array->elementCount);

    ASTLinkedListRef list = array->list;
    if (index > 0) {
        for (Index offset = 0; offset < index - 1; offset++) {
            assert(list->next);
            list = list->next;
        }
    }

    if (list->next) {
        list->next = list->next->next;
    }

    array->elementCount -= 1;
}

void ASTArrayRemoveAllElements(ASTArrayRef array) {
    array->list         = NULL;
    array->elementCount = 0;
}

bool ASTArrayContainsElement(ASTArrayRef array, ASTArrayPredicate predicate, void *element) {
    ASTLinkedListRef list = array->list;
    while (list) {
        if (predicate(list->node, element)) {
            return true;
        }

        list = list->next;
    }

    return false;
}

bool ASTArrayIsEqual(ASTArrayRef lhs, ASTArrayRef rhs) {
    if (lhs->elementCount != rhs->elementCount) {
        return false;
    }

    ASTLinkedListRef lhsList = lhs->list;
    ASTLinkedListRef rhsList = rhs->list;
    while (lhsList) {
        if (lhsList->node != rhsList->node) {
            return false;
        }

        lhsList = lhsList->next;
        rhsList = rhsList->next;
    }

    return true;
}
