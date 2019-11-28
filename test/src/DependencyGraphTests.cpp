#include <gtest/gtest.h>
#include <JellyCore/JellyCore.h>

TEST(DependencyGraph, DependencyGraphCreateEmpty) {
    DependencyGraphRef graph = DependencyGraphCreate(AllocatorGetSystemDefault());
    EXPECT_NE(graph, nullptr);
    DependencyGraphDestroy(graph);
}

TEST(DependencyGraph, DependencyGraphSortWithDepth1) {
    DependencyGraphRef graph = DependencyGraphCreate(AllocatorGetSystemDefault());
    EXPECT_NE(graph, nullptr);

    StringRef identifier = StringCreate(AllocatorGetSystemDefault(), "A");
    DependencyGraphInsertNode(graph, identifier);
    StringDestroy(identifier);

    identifier = StringCreate(AllocatorGetSystemDefault(), "B");
    DependencyGraphInsertNode(graph, identifier);
    StringDestroy(identifier);

    identifier = StringCreate(AllocatorGetSystemDefault(), "C");
    DependencyGraphInsertNode(graph, identifier);
    StringDestroy(identifier);

    identifier = StringCreate(AllocatorGetSystemDefault(), "D");
    DependencyGraphInsertNode(graph, identifier);
    StringDestroy(identifier);

    Bool hasCyclicDependencies;
    ArrayRef result = DependencyGraphGetIdentifiersInTopologicalOrder(graph, &hasCyclicDependencies);
    EXPECT_EQ(ArrayGetElementCount(result), 4);
    EXPECT_EQ(hasCyclicDependencies, false);
    EXPECT_STREQ("A", StringGetCharacters(*((StringRef*)ArrayGetElementAtIndex(result, 0))));
    EXPECT_STREQ("B", StringGetCharacters(*((StringRef*)ArrayGetElementAtIndex(result, 1))));
    EXPECT_STREQ("C", StringGetCharacters(*((StringRef*)ArrayGetElementAtIndex(result, 2))));
    EXPECT_STREQ("D", StringGetCharacters(*((StringRef*)ArrayGetElementAtIndex(result, 3))));

    DependencyGraphDestroy(graph);
}

TEST(DependencyGraph, DependencyGraphSortWithDepth2) {
    DependencyGraphRef graph = DependencyGraphCreate(AllocatorGetSystemDefault());
    EXPECT_NE(graph, nullptr);

    StringRef identifier = StringCreate(AllocatorGetSystemDefault(), "A");
    DependencyGraphNodeRef a =  DependencyGraphInsertNode(graph, identifier);
    StringDestroy(identifier);

    identifier = StringCreate(AllocatorGetSystemDefault(), "B");
    DependencyGraphNodeRef b = DependencyGraphInsertNode(graph, identifier);
    StringDestroy(identifier);

    identifier = StringCreate(AllocatorGetSystemDefault(), "C");
    DependencyGraphNodeRef c = DependencyGraphInsertNode(graph, identifier);
    StringDestroy(identifier);

    identifier = StringCreate(AllocatorGetSystemDefault(), "D");
    DependencyGraphNodeRef d = DependencyGraphInsertNode(graph, identifier);
    StringDestroy(identifier);

    identifier = StringCreate(AllocatorGetSystemDefault(), "A");
    Bool inserted = DependencyGraphAddDependency(graph, d, identifier);
    EXPECT_EQ(inserted, true);
    StringDestroy(identifier);

    identifier = StringCreate(AllocatorGetSystemDefault(), "C");
    inserted = DependencyGraphAddDependency(graph, a, identifier);
    EXPECT_EQ(inserted, true);
    StringDestroy(identifier);

    identifier = StringCreate(AllocatorGetSystemDefault(), "B");
    inserted = DependencyGraphAddDependency(graph, c, identifier);
    EXPECT_EQ(inserted, true);
    StringDestroy(identifier);

    Bool hasCyclicDependencies;
    ArrayRef result = DependencyGraphGetIdentifiersInTopologicalOrder(graph, &hasCyclicDependencies);
    EXPECT_EQ(ArrayGetElementCount(result), 4);
    EXPECT_EQ(hasCyclicDependencies, false);
    EXPECT_STREQ("D", StringGetCharacters(*((StringRef*)ArrayGetElementAtIndex(result, 0))));
    EXPECT_STREQ("A", StringGetCharacters(*((StringRef*)ArrayGetElementAtIndex(result, 1))));
    EXPECT_STREQ("C", StringGetCharacters(*((StringRef*)ArrayGetElementAtIndex(result, 2))));
    EXPECT_STREQ("B", StringGetCharacters(*((StringRef*)ArrayGetElementAtIndex(result, 3))));

    DependencyGraphDestroy(graph);
}

TEST(DependencyGraph, DependencyGraphSortWithCyclicDependencyDepth2) {
    DependencyGraphRef graph = DependencyGraphCreate(AllocatorGetSystemDefault());
    EXPECT_NE(graph, nullptr);

    StringRef identifier = StringCreate(AllocatorGetSystemDefault(), "A");
    DependencyGraphNodeRef a =  DependencyGraphInsertNode(graph, identifier);
    StringDestroy(identifier);

    identifier = StringCreate(AllocatorGetSystemDefault(), "B");
    DependencyGraphNodeRef b = DependencyGraphInsertNode(graph, identifier);
    StringDestroy(identifier);

    identifier = StringCreate(AllocatorGetSystemDefault(), "C");
    DependencyGraphNodeRef c = DependencyGraphInsertNode(graph, identifier);
    StringDestroy(identifier);

    identifier = StringCreate(AllocatorGetSystemDefault(), "D");
    DependencyGraphNodeRef d = DependencyGraphInsertNode(graph, identifier);
    StringDestroy(identifier);

    identifier = StringCreate(AllocatorGetSystemDefault(), "A");
    Bool inserted = DependencyGraphAddDependency(graph, d, identifier);
    EXPECT_EQ(inserted, true);
    StringDestroy(identifier);

    identifier = StringCreate(AllocatorGetSystemDefault(), "C");
    inserted = DependencyGraphAddDependency(graph, a, identifier);
    EXPECT_EQ(inserted, true);
    StringDestroy(identifier);

    identifier = StringCreate(AllocatorGetSystemDefault(), "B");
    inserted = DependencyGraphAddDependency(graph, c, identifier);
    EXPECT_EQ(inserted, true);
    StringDestroy(identifier);

    identifier = StringCreate(AllocatorGetSystemDefault(), "D");
    inserted = DependencyGraphAddDependency(graph, b, identifier);
    EXPECT_EQ(inserted, true);
    StringDestroy(identifier);

    Bool hasCyclicDependencies;
    ArrayRef result = DependencyGraphGetIdentifiersInTopologicalOrder(graph, &hasCyclicDependencies);
    EXPECT_EQ(ArrayGetElementCount(result), 0);
    EXPECT_EQ(hasCyclicDependencies, true);

    DependencyGraphDestroy(graph);
}
