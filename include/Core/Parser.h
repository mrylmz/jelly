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

#include "Core/ASTContext.h"
#include "Core/Diagnostic.h"
#include "Core/Lexer.h"
#include "Core/Operator.h"

// @Incomplete Parse all leading and trailing trivia of a token in a useful fashion without storing the contents
//             build a request api for the contents of trivia tokens like comments...
//             rewrite the AST Printer to emit the original source file given into the compiler
//             but appending the contents of #load files if the AST is populated further with source files...

struct Parser {
    Lexer* lexer;
    ASTContext* context;
    DiagnosticEngine* diag;
    ASTNode* parent = nullptr;
    Operator op;
    Token token;
    bool silentErrors = false;

    Parser(Lexer* lexer, ASTContext* context, DiagnosticEngine* diag);

    void parseAllTopLevelNodes();

    template<typename ...Args>
    void report(DiagnosticLevel level, const char* format, Args&&... args);
};

template<typename ...Args>
void Parser::report(DiagnosticLevel level, const char *format, Args&&... args) {
    if (silentErrors) { return; }

    diag->report(level, format, args...);
}
