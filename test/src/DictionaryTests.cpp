#include <gtest/gtest.h>
#include <JellyCore/JellyCore.h>

TEST(Dictionary, CStringDictionaryCreate) {
    DictionaryRef dictionary = CStringDictionaryCreate(AllocatorGetSystemDefault(), 2);
    EXPECT_NE(dictionary, nullptr);
    DictionaryDestroy(dictionary);
}

TEST(Dictionary, CStringDictionaryInsertAndLookup) {
    Int64 elements[] = {123, 456, 789};
    const Int64 *value;

    DictionaryRef dictionary = CStringDictionaryCreate(AllocatorGetSystemDefault(), 2);
    DictionaryInsert(dictionary, "Hello", &elements[0], sizeof(Int64));
    DictionaryInsert(dictionary, "World", &elements[1], sizeof(Int64));
    value = (const Int64 *)DictionaryLookup(dictionary, "Hello");
    EXPECT_NE(value, nullptr);
    EXPECT_EQ(123, *value);
    value = (const Int64 *)DictionaryLookup(dictionary, "World");
    EXPECT_NE(value, nullptr);
    EXPECT_EQ(456, *value);
    value = (const Int64 *)DictionaryLookup(dictionary, "Test");
    EXPECT_EQ(value, nullptr);
    value = (const Int64 *)DictionaryLookup(dictionary, "Other");
    EXPECT_EQ(value, nullptr);
    DictionaryInsert(dictionary, "Other", &elements[2], sizeof(Int64));
    value = (const Int64 *)DictionaryLookup(dictionary, "Other");
    EXPECT_NE(value, nullptr);
    EXPECT_EQ(789, *value);
    DictionaryDestroy(dictionary);
}

TEST(Dictionary, CStringDictionaryInsertNull) {
    Int64 elements[] = {123, 456, 789};
    const Int64 *value;

    DictionaryRef dictionary = CStringDictionaryCreate(AllocatorGetSystemDefault(), 2);
    DictionaryInsert(dictionary, "A", &elements[0], sizeof(Int64));
    DictionaryInsert(dictionary, "B", &elements[1], sizeof(Int64));
    DictionaryInsert(dictionary, "C", &elements[2], sizeof(Int64));
    value = (const Int64 *)DictionaryLookup(dictionary, "A");
    EXPECT_NE(value, nullptr);
    EXPECT_EQ(123, *value);
    value = (const Int64 *)DictionaryLookup(dictionary, "B");
    EXPECT_NE(value, nullptr);
    EXPECT_EQ(456, *value);
    value = (const Int64 *)DictionaryLookup(dictionary, "C");
    EXPECT_NE(value, nullptr);
    EXPECT_EQ(789, *value);
    DictionaryInsert(dictionary, "B", NULL, sizeof(Int64));
    value = (const Int64 *)DictionaryLookup(dictionary, "A");
    EXPECT_NE(value, nullptr);
    EXPECT_EQ(123, *value);
    value = (const Int64 *)DictionaryLookup(dictionary, "B");
    EXPECT_EQ(value, nullptr);
    value = (const Int64 *)DictionaryLookup(dictionary, "C");
    EXPECT_NE(value, nullptr);
    EXPECT_EQ(789, *value);
    DictionaryInsert(dictionary, "A", NULL, sizeof(Int64));
    value = (const Int64 *)DictionaryLookup(dictionary, "A");
    EXPECT_EQ(value, nullptr);
    value = (const Int64 *)DictionaryLookup(dictionary, "B");
    EXPECT_EQ(value, nullptr);
    value = (const Int64 *)DictionaryLookup(dictionary, "C");
    EXPECT_NE(value, nullptr);
    EXPECT_EQ(789, *value);
    DictionaryDestroy(dictionary);
}

TEST(Dictionary, CStringDictionaryRemove) {
    Int64 elements[] = {123, 456, 789};
    const Int64 *value;

    DictionaryRef dictionary = CStringDictionaryCreate(AllocatorGetSystemDefault(), 2);
    DictionaryInsert(dictionary, "A", &elements[0], sizeof(Int64));
    DictionaryInsert(dictionary, "A", &elements[0], sizeof(Int64));
    DictionaryInsert(dictionary, "B", &elements[1], sizeof(Int64));
    DictionaryInsert(dictionary, "C", &elements[2], sizeof(Int64));
    value = (const Int64 *)DictionaryLookup(dictionary, "A");
    EXPECT_NE(value, nullptr);
    EXPECT_EQ(123, *value);
    value = (const Int64 *)DictionaryLookup(dictionary, "B");
    EXPECT_NE(value, nullptr);
    EXPECT_EQ(456, *value);
    value = (const Int64 *)DictionaryLookup(dictionary, "C");
    EXPECT_NE(value, nullptr);
    EXPECT_EQ(789, *value);
    DictionaryRemove(dictionary, "B");
    value = (const Int64 *)DictionaryLookup(dictionary, "A");
    EXPECT_NE(value, nullptr);
    EXPECT_EQ(123, *value);
    value = (const Int64 *)DictionaryLookup(dictionary, "B");
    EXPECT_EQ(value, nullptr);
    value = (const Int64 *)DictionaryLookup(dictionary, "C");
    EXPECT_NE(value, nullptr);
    EXPECT_EQ(789, *value);
    DictionaryRemove(dictionary, "A");
    value = (const Int64 *)DictionaryLookup(dictionary, "A");
    EXPECT_EQ(value, nullptr);
    value = (const Int64 *)DictionaryLookup(dictionary, "B");
    EXPECT_EQ(value, nullptr);
    value = (const Int64 *)DictionaryLookup(dictionary, "C");
    EXPECT_NE(value, nullptr);
    EXPECT_EQ(789, *value);
    DictionaryDestroy(dictionary);
}
