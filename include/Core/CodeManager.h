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

#include <string>

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/StringRef.h>

struct CodeManager {
    std::string workingDirectory;
    llvm::SmallVector<std::string, 0> currentDirectoryStack;
    llvm::SmallVector<std::string, 0> sourceFilePaths;
    llvm::SmallVector<std::string, 0> sourceContents;

    unsigned bufferFilePathIndex = 0;
    unsigned parseFileIndex = 0;
    unsigned preprocessNodeIndex = 0;

    ASTContext context;
    DiagnosticEngine diag;

    CodeManager(std::string workingDirectory, DiagnosticHandler* diagHandler);

    static std::string getCurrentWorkingDirectory();

    void addSourceFile(llvm::StringRef sourceFilePath);
    void addSourceText(llvm::StringRef sourceText);

    void loadPendingSourceFiles();

    void parseAST();
    void printAST();
    void typecheckAST();

    ASTExpr* evaluateConstExpr(ASTExpr* expr);
};
