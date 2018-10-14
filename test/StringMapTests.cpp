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

const char* strings[] = {
    "Black Hopper",
    "Abstract Crawler",
    "Storm Katydid",
    "Putrid Psocid",
    "Entangling Millipede",
    "Hill Mite",
    "Rigid Bee",
    "Gilded Flatworm",
    "Battle Maggot",
    "Wax Locust",
    "The Broken Globe",
    "The Brave Favor",
    "The Curly Charm",
    "The Colossal Breath",
    "The Secret Belle",
    "The Gifted Petal Bracelet",
    "The Velvet Flower Tiara",
    "The Shadow Root Bracelet",
    "The Royal Valor Bracelet",
    "The Innocent Blossom Ring"
};

size_t count = sizeof(strings) / sizeof(String*);

TEST(StringMap, short_strings) {
    StringMap<size_t, 64> map;
    size_t value;

    for (size_t i = 0; i < count; i++) {
        String string(strings[i]);

        map.set(string, string.hash() % 5);
        EXPECT_TRUE(map.get(string, value));
        EXPECT_EQ(value, string.hash() % 5);

        map.set(string, string.hash());
        EXPECT_TRUE(map.get(string, value));
        EXPECT_EQ(value, string.hash());
    }

    for (size_t i = 0; i < count; i++) {
        if (i % 2 == 1) { continue; }

        String string(strings[i]);

        map.remove(string);
    }

    for (size_t i = 0; i < count; i++) {
        String string(strings[i]);

        if (i % 2 == 1) {
            EXPECT_TRUE(map.get(string, value));
            EXPECT_EQ(value, string.hash());
        } else {
            EXPECT_FALSE(map.get(string, value));
        }
    }
}
