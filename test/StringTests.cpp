//
// MIT License
//
// Copyright (c) 2018 Murat Yilmaz
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include <gtest/gtest.h>
#include <Basic/Basic.h>
#include <stdio.h>

#define SUIT String

TEST(SUIT, init) {
    String string;
    EXPECT_EQ(string.buffer_start, nullptr);
    EXPECT_EQ(string.buffer_length, 0);
}

TEST(SUIT, init_buffer) {
    const char* cstring = "Hello World!";
    String string(cstring);
    EXPECT_EQ(string.buffer_start, cstring);
    EXPECT_EQ(string.buffer_length, 12);
}

TEST(SUIT, is_equal) {
    const char* equal_string = "My super cool test string!";
    String string(equal_string);
    EXPECT_TRUE(string.is_equal(equal_string));

    const char* not_equal_string = "My super cool unique string!";
    EXPECT_FALSE(string.is_equal(not_equal_string));
}

TEST(SUIT, init_buffer_and_length) {
    const char* cstring = "Hello World!";
    String string(cstring, 5);
    EXPECT_EQ(string.buffer_start, cstring);
    EXPECT_EQ(string.buffer_length, 5);
    EXPECT_TRUE(string.is_equal("Hello"));
}

TEST(SUIT, copy_buffer) {
    const char* cstring = "Hello World!";
    String string(cstring);
    EXPECT_EQ(cstring, string.buffer_start);
    char* copy = string.copy_buffer();
    EXPECT_NE(cstring, copy);
    std::free(copy);
}

TEST(SUIT, is_empty) {
    String string;
    EXPECT_TRUE(string.is_empty());
    string = "Hello World";
    EXPECT_FALSE(string.is_empty());
}

TEST(SUIT, has_prefix) {
    String string("SuperPrefix");
    EXPECT_TRUE(string.has_prefix("Super"));
    EXPECT_FALSE(string.has_prefix("Prefix"));
}

TEST(SUIT, has_suffix) {
    String string("SuperSuffix");
    EXPECT_TRUE(string.has_suffix("Suffix"));
    EXPECT_FALSE(string.has_suffix("Super"));
}

TEST(SUIT, slice) {
    String string("Hello World!");
    String slice = string.slice(6, 10);
    EXPECT_FALSE(string.is_equal(slice));
    EXPECT_TRUE(slice.is_equal("World"));
}

TEST(SUIT, prefix) {
    String string("Hello World!");
    String slice = string.prefix(5);
    EXPECT_FALSE(string.is_equal(slice));
    EXPECT_TRUE(slice.is_equal("Hello"));
}

TEST(SUIT, suffix) {
    String string("Hello World!");
    String slice = string.suffix(6);
    EXPECT_FALSE(string.is_equal(slice));
    EXPECT_TRUE(slice.is_equal("World!"));
}

TEST(SUIT, hash) {
    String lhs("Hello");
    String rhs("Hello!");
    EXPECT_NE(lhs.hash(), rhs.hash());
    rhs = "Hello";
    EXPECT_EQ(lhs.hash(), rhs.hash());
}

TEST(SUIT, convert_to_int) {
    uint64_t value;
    uint64_t comparison;
    String number("1298318237");
    EXPECT_TRUE(number.convert_to_int(value));
    EXPECT_EQ(value, 1298318237);

    number = "0xFF1273823";
    comparison = 0xFF1273823;
    EXPECT_TRUE(number.convert_to_int(value));
    EXPECT_EQ(value, comparison);

    number = "0o5612315234";
    EXPECT_TRUE(number.convert_to_int(value));
    EXPECT_EQ(value, 774478492);

    number = "0b1010100101010";
    comparison = 0b1010100101010;
    EXPECT_TRUE(number.convert_to_int(value));
    EXPECT_EQ(value, comparison);

    number = "0xFFKK9a8.";
    EXPECT_FALSE(number.convert_to_int(value));

    number = "abc12";
    EXPECT_FALSE(number.convert_to_int(value));

    number = "0o";
    EXPECT_FALSE(number.convert_to_int(value));
}

#undef SUIT
