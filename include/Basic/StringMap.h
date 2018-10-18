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

#pragma once

#include <string>

#include "Basic/String.h"
#include "Basic/Hasher.h"

template <typename Value, size_t capacity>
struct StringMap {
    StringMap() {
        memset(bucket, 0, sizeof(Element*) * capacity);
    }

    ~StringMap() {
        for (size_t i = 0; i < capacity; i++) {
            Element* element = bucket[i];

            while (element != nullptr) {
                Element* previous = element;
                element = element->next;
                free(previous);
            }
        }
    }

    bool get(const String& key, Value& value) {
        size_t hash      = key.hash();
        size_t index     = hash % capacity;
        Element* element = bucket[index];

        while (element != nullptr) {
            if (element->hash != hash || !element->key.is_equal(key)) {
                element = element->next;
                continue;
            }

            value = element->value;
            return true;
        }

        return false;
    }

    bool has(const String& key) {
        size_t hash      = key.hash();
        size_t index     = hash % capacity;
        Element* element = bucket[index];

        while (element != nullptr) {
            if (element->hash != hash || !element->key.is_equal(key)) {
                element = element->next;
                continue;
            }

            return true;
        }

        return false;
    }

    void set(const String& key, const Value& value) {
        size_t hash      = key.hash();
        size_t index     = hash % capacity;
        Element* element = bucket[index];

        while (element != nullptr) {
            if (element->hash != hash || !element->key.is_equal(key)) {
                element = element->next;
                continue;
            }

            element->value = value;
            return;
        }

        element = (Element*)malloc(sizeof(Element));
        if (element == nullptr) {
            fatal_error("Memory allocation failed!");
        }

        element->hash = hash;
        element->key = key;
        element->value = value;
        element->next = bucket[index];
        bucket[index] = element;
    }

    void remove(const String& key) {
        size_t hash       = key.hash();
        size_t index      = hash % capacity;
        Element* element  = bucket[index];
        Element* previous = nullptr;

        while (element != nullptr) {
            if (element->hash != hash || !element->key.is_equal(key)) {
                previous = element;
                element  = element->next;
                continue;
            }

            if (previous != nullptr) {
                previous->next = element->next;
            } else {
                bucket[index] = element->next;
            }

            free(element);
            element = nullptr;
        }
    }

private:
    struct Element {
        size_t   hash;
        String   key;
        Value    value;
        Element* next;
    };

    Element* bucket[capacity];
};
