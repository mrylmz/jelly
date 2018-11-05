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

#include "AST/ASTContext.h"

static size_t node_sizes[] = {
    sizeof(ASTNode),
    sizeof(ASTStatement),
    sizeof(ASTDeclaration),
    sizeof(ASTExpression),
    sizeof(ASTUnaryExpression),
    sizeof(ASTBinaryExpression),
    sizeof(ASTIdentifier),
    sizeof(ASTType),
    sizeof(ASTLiteral),
    sizeof(ASTDirective),
    sizeof(ASTLoad),
    sizeof(ASTParameter),
    sizeof(ASTBlock),
    sizeof(ASTFuncSignature),
    sizeof(ASTFunc),
    sizeof(ASTVariable),
    sizeof(ASTStruct),
    sizeof(ASTEnumElement),
    sizeof(ASTEnum),
    sizeof(ASTControl),
    sizeof(ASTSwitchCase),
    sizeof(ASTDefer),
    sizeof(ASTFor),
    sizeof(ASTGuard),
    sizeof(ASTIf),
    sizeof(ASTSwitch),
    sizeof(ASTLoop),
    sizeof(ASTCall),
    sizeof(ASTSubscript)
};

ASTContext::ASTContext() {
    size_t container_size = node_sizes[0];
    for (auto i = 1; i < sizeof(node_sizes) / sizeof(size_t); i++) {
        container_size = std::max(container_size, node_sizes[i]);
    }

    page_size = sysconf(_SC_PAGESIZE);
    node_size = 1;
    while (node_size < container_size) node_size *= 2;
    node_count = 0;
    nodes_per_page = page_size / node_size;

    while (nodes_per_page <= 0) {
        page_size *= 2;
        nodes_per_page = page_size / node_size;
    }

    void* buffer = malloc(page_size);
    if (buffer == nullptr) {
        fatal_error("Memory allocation failed!");
    }

    node_pages.push_back((uint8_t*)buffer);

    root = new (this) ASTBlock;
}

ASTContext::~ASTContext() {
    for (auto it = lexeme_values.begin(); it != lexeme_values.end(); it++) {
        free((void*)it->buffer_start);
    }

    for (auto it = node_pages.begin(); it != node_pages.end(); it++) {
        free(*it);
    }
}

void* ASTContext::alloc_node()  {
    size_t page_index = node_count / nodes_per_page;
    size_t node_index = node_count - page_index * nodes_per_page;

    if (page_index >= node_pages.size()) {
        void* buffer = malloc(page_size);
        if (buffer == nullptr) {
            fatal_error("Memory allocation failed!");
        }

        node_pages.push_back(buffer);
    }

    uint8_t* buffer  = (uint8_t*)node_pages.at(page_index);
    uint8_t* pointer = buffer + node_index * node_size;

    node_count += 1;
    return pointer;
}

ASTLexeme ASTContext::get_lexeme(const String& text) {
    ASTLexeme lexeme;

    if (lexeme_map.get(text, lexeme.index)) {
        return lexeme;
    }

    String copy(text.copy_buffer(), text.buffer_length);

    lexeme.index = lexeme_values.size();
    lexeme_values.push_back(copy);
    lexeme_map.set(copy, lexeme.index);
    return lexeme;
}

String ASTContext::get_lexeme_text(const ASTLexeme& lexeme) const {
    assert(lexeme.index >= 0 && "Invalid lexeme given!");
    return lexeme_values.at(lexeme.index);
}
