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

#include "Core/CodeManager.h"
#include "Core/Lexer.h"
#include "Core/Parser.h"
#include "Core/Sema.h"

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>

CodeManager::CodeManager(DiagnosticHandler* diagHandler) : diag(DiagnosticEngine(diagHandler)) {}

std::string CodeManager::getNativePath(llvm::StringRef path) {
    llvm::SmallVector<char, 64> buffer(path.begin(), path.end());
    llvm::sys::path::native(buffer);
    return std::string(buffer.begin(), buffer.size());
}

void CodeManager::addSourceFile(llvm::StringRef sourceFilePath) {
    for (auto filePath : sourceFilePaths) {
        if (filePath == sourceFilePath) {
            return diag.report(DIAG_ERROR, "Cannot load source file at path '{0}' twice", sourceFilePath);
        }
    }
    sourceFilePaths.push_back(sourceFilePath);

    auto buffer = sourceManager.addIncludeFile(sourceFilePath);
    if (!buffer.isValid()) {
        diag.report(DIAG_ERROR, "Couldn't load file at path '{0}'", sourceFilePath);
        return;
    }

    sourceBuffers.push_back(buffer);
}

void CodeManager::addSourceText(llvm::StringRef sourceText) {
    auto buffer = sourceManager.addSourceBuffer(sourceText);
    if (!buffer.isValid()) {
        diag.report(DIAG_ERROR, "Couldn't add source text to CodeManager!");
        return;
    }

    sourceBuffers.push_back(buffer);
}

void CodeManager::parseAST() {
    while (parseFileIndex < sourceBuffers.size()) {
        auto buffer = sourceBuffers[parseFileIndex];
        {
            Lexer lexer(buffer);
            Parser parser(&lexer, &context, &diag);
            parser.parseAllTopLevelNodes();

            auto module = context.getModule();
            for (auto it = module->declsBegin() + preprocessDeclIndex; it != module->declsEnd(); it++) {
                if ((*it)->kind == AST_LOAD_DIRECTIVE) {
                    auto loadDirective = reinterpret_cast<ASTLoadDirective*>(*it);
                    auto loadFilePath = getNativePath(loadDirective->loadFilePath);

                    // @Incomplete Add source location information of ASTLoadDirective and enable includes of local files in subdirectories
                    addSourceFile(loadFilePath);

                    if (diag.hasErrors()) {
                        return;
                    }
                }

                preprocessDeclIndex += 1;
            }
        }
        parseFileIndex += 1;
    }
}

void CodeManager::typecheckAST() {
    parseAST();
    if (diag.hasErrors()) { return; }

    Sema sema(this);
    sema.validateAST();
}
