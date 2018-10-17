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

#include "AST/ASTNodes.h"

#include <Basic/Basic.h>
#include <vector>
#include <unistd.h>
#include <iostream>
#include <map>
#include <string>

#warning Isn't there a cleaner solution to the node size problem then ASTNodeContainer ??

struct ASTContext {
    template<typename Element>
    using Array = std::vector<Element>;

    ASTContext();
    ~ASTContext();

    void* alloc_node();

    ASTLexeme get_lexeme(const String& text);
    String    get_lexeme_text(const ASTLexeme& lexeme) const;

    ASTBlock* root;

private:
    StringMap<int64_t, 4 * 1024> lexeme_map;
    Array<String>                lexeme_values;

    size_t       page_size;
    size_t       node_size;
    size_t       node_count;
    size_t       nodes_per_page;
    Array<void*> node_pages;
};
