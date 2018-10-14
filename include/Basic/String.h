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

#include <stddef.h>
#include <stdint.h>

struct Hasher;

struct String {
    const char* buffer_start;
    size_t      buffer_length;

    String();
    String(const char *buffer);
    String(const char *buffer, size_t length);

    ~String();

    char*       copy_buffer() const;

    bool        is_empty() const;
    bool        is_equal(String other) const;

    bool        has_prefix(String prefix) const;
    bool        has_suffix(String suffix) const;

    String      slice(size_t start, size_t end) const;
    String      prefix(size_t length) const;
    String      suffix(size_t length) const;

    size_t      hash() const;
    void        hash_into(Hasher* hasher) const;

    bool        convert_to_int(uint64_t& result) const;
    bool        convert_to_int(uint32_t radix, uint64_t& result) const;
    bool        convert_to_double(double &result) const;

    char operator [] (size_t index) const;
};

inline bool operator == (String lhs, String rhs);
inline bool operator != (String lhs, String rhs);
