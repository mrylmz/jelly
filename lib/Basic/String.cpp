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

#include <string>
#include <math.h>

#include "Basic/String.h"
#include "Basic/Hasher.h"

String::String() :
buffer_start(nullptr),
buffer_length(0) {
}

String::String(const char *buffer) :
buffer_start(buffer),
buffer_length(0) {
    if (buffer != nullptr) {
        buffer_length = strlen(buffer);
    }
}

String::String(const char *buffer, size_t length) :
buffer_start(buffer),
buffer_length(length) {
}

String::~String() {
}

char* String::copy_buffer() const {
    assert(buffer_start != nullptr);

    char* copy_memory = (char*)malloc(sizeof(char) * buffer_length);
    assert(copy_memory != nullptr);

    memcpy(copy_memory, buffer_start, buffer_length);
    return copy_memory;
}

bool String::is_empty() const {
    return buffer_length == 0;
}

bool String::is_equal(String other) const {
    if (buffer_length != other.buffer_length) {
        return false;
    }

    if (buffer_length == 0) {
        return true;
    }

    return memcmp(buffer_start, other.buffer_start, buffer_length) == 0;
}

bool String::has_prefix(String prefix) const {
    if (buffer_length == 0 || prefix.buffer_length == 0 || buffer_length < prefix.buffer_length) {
        return false;
    }

    return memcmp(buffer_start, prefix.buffer_start, prefix.buffer_length) == 0;
}

bool String::has_suffix(String suffix) const {
    if (buffer_length == 0 || suffix.buffer_length == 0 || buffer_length < suffix.buffer_length) {
        return false;
    }

    return memcmp(buffer_start + buffer_length - suffix.buffer_length, suffix.buffer_start, suffix.buffer_length) == 0;
}

String String::slice(size_t start, size_t end) const {
    assert(start < buffer_length);
    assert(end < buffer_length);
    return String(buffer_start + start, end - start + 1);
}

String String::prefix(size_t length) const {
    assert(length <= buffer_length);
    return String(buffer_start, length);
}

String String::suffix(size_t length) const {
    assert(length <= buffer_length);
    return String(buffer_start + buffer_length - length, length);
}

size_t String::hash() const {
    Hasher hasher;
    hasher.combine(this);
    return hasher.finalize();
}

void String::hash_into(Hasher* hasher) const {
    for (size_t i = 0; i < buffer_length; i++)
        hasher->combine(buffer_start[i]);

    hasher->combine(buffer_length);
}

bool String::convert_to_int(uint64_t &result) const {
    if (has_prefix("0b") || has_prefix("0B")) {
        return suffix(buffer_length - 2).convert_to_int(2, result);
    }

    if (has_prefix("0o") || has_prefix("0O")) {
        return suffix(buffer_length - 2).convert_to_int(8, result);
    }

    if (has_prefix("0x") || has_prefix("0X")) {
        return suffix(buffer_length - 2).convert_to_int(16, result);
    }
    
    return convert_to_int(10, result);
}

// TODO: Return error codes for better error-reporting in Parser
bool String::convert_to_int(uint32_t radix, uint64_t& result) const {
    assert(2 <= radix && radix <= 36);

    if (is_empty()) {
        return false;
    }

    result = 0;

    const char* buffer_ptr = buffer_start;
    const char* buffer_end = buffer_start + buffer_length;

    while (buffer_ptr != buffer_end) {
        uint64_t next;

        if ('0' <= *buffer_ptr && *buffer_ptr <= '9') {
            next = *buffer_ptr - '0';
        } else if ('a' <= *buffer_ptr && *buffer_ptr <= 'z') {
            next = *buffer_ptr - 'a' + 10;
        } else if ('A' <= *buffer_ptr && *buffer_ptr <= 'Z') {
            next = *buffer_ptr - 'A' + 10;
        } else {
            return false;
        }

        if (next >= radix) {
            return false;
        }

        // Checking for overflow
        uint64_t next_result = result * radix + next;
        if (next_result / radix < result) {
            return false;
        }

        result = next_result;
        buffer_ptr += 1;
    }

    return true;
}

bool String::convert_to_double(double &result) const {
    std::string text(buffer_start, buffer_start + buffer_length);
    char*       double_end;
    const char* ctext = text.c_str();

    result = strtod(ctext, &double_end);
    if (ctext + buffer_length != double_end) {
        return false;
    }

    return true;
}

char String::operator [] (size_t index) const {
    assert(index < buffer_length);
    return buffer_start[index];
}

inline bool operator == (String lhs, String rhs) {
    return lhs.is_equal(rhs);
}

inline bool operator != (String lhs, String rhs) {
    return !(lhs == rhs);
}
