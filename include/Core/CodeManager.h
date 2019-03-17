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
#include <Basic/Basic.h>
#include <AST/AST.h>

struct CodeManager {
    jelly::SourceManager sourceManager;
    jelly::SmallVector<std::string, 0> sourceFilePaths;
    jelly::SmallVector<jelly::SourceBuffer, 0> sourceBuffers;

    unsigned parseFileIndex = 0;
    unsigned preprocessDeclIndex = 0;

    jelly::AST::Context context;
    jelly::DiagnosticEngine diagnosticEngine;

    CodeManager(jelly::DiagnosticHandler* diagnosticHandler);

    void addSourceFile(jelly::StringRef sourceFilePath);
    void addSourceText(jelly::StringRef sourceText);

    void parseAST();
    void printAST();
    void typecheckAST();

    jelly::AST::Expression* evaluateConstantExpression(jelly::AST::Expression* expression);

private:
    std::string getNativePath(jelly::StringRef path);
};
