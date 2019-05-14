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
#include "Core/Sema.h"

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <Parse/Parse.h>

using namespace jelly;
using namespace jelly::AST;
using namespace jelly::Parse;

CodeManager::CodeManager(DiagnosticHandler* diagnosticHandler) : diagnosticEngine(DiagnosticEngine(diagnosticHandler)) {}

std::string CodeManager::getNativePath(jelly::StringRef path) {
    jelly::SmallVector<char, 64> buffer(path.begin(), path.end());
    jelly::native(buffer);
    return std::string(buffer.begin(), buffer.size());
}

void CodeManager::addSourceFile(jelly::StringRef sourceFilePath) {
    for (auto filePath : sourceFilePaths) {
        if (filePath == sourceFilePath) {
            return diagnosticEngine.report(Diagnostic::Level::Error, "Cannot load source file at path '{0}' twice", sourceFilePath);
        }
    }
    sourceFilePaths.push_back(sourceFilePath);

    auto buffer = sourceManager.addIncludeFile(sourceFilePath);
    if (!buffer.isValid()) {
        diagnosticEngine.report(Diagnostic::Level::Error, "Couldn't load file at path '{0}'", sourceFilePath);
        return;
    }

    sourceBuffers.push_back(buffer);
}

void CodeManager::addSourceText(jelly::StringRef sourceText) {
    auto buffer = sourceManager.addSourceBuffer(sourceText);
    if (!buffer.isValid()) {
        diagnosticEngine.report(Diagnostic::Level::Error, "Couldn't add source text to CodeManager!");
        return;
    }

    sourceBuffers.push_back(buffer);
}

void CodeManager::parseAST() {
    while (parseFileIndex < sourceBuffers.size()) {
        auto buffer = sourceBuffers[parseFileIndex];
        {
            Lexer lexer(&context, buffer);
            Parser parser(&lexer, &diagnosticEngine);
            parser.parseAllTopLevelNodes();

            auto children = context.getModule()->getChildren();
            for (auto it = children.begin() + preprocessChildIndex; it != children.end(); it++) {
                if ((*it)->isLoadDirective()) {
                    auto declaration = reinterpret_cast<LoadDirective*>(*it);
                    auto sourceFilePath = getNativePath(declaration->getSourceFilePath());

                    // @Incomplete Add source location information of LoadDirective and enable includes of local files in subdirectories
                    addSourceFile(sourceFilePath);

                    if (diagnosticEngine.getReportedErrorCount() > 0 || diagnosticEngine.getReportedFatalCount() > 0) {
                        return;
                    }
                }

                preprocessChildIndex += 1;
            }
        }
        parseFileIndex += 1;
    }
}

void CodeManager::typecheckAST() {
    parseAST();

    if (diagnosticEngine.getReportedErrorCount() > 0 || diagnosticEngine.getReportedFatalCount() > 0) {
        return;
    }

    Sema sema(this);
    sema.validateAST();
}