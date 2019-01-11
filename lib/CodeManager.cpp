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

#ifdef WINDOWS
#include <direct.h>
#define __getCurrentWorkingDirectory _getcwd
#else
#include <unistd.h>
#define __getCurrentWorkingDirectory getcwd
#endif

CodeManager::CodeManager(std::string workingDirectory, DiagnosticHandler* diagHandler) :
workingDirectory(workingDirectory), diag(DiagnosticEngine(diagHandler)) {
    currentDirectoryStack.push_back(workingDirectory);
}

std::string CodeManager::getCurrentWorkingDirectory() {
    char buffer[FILENAME_MAX];
    __getCurrentWorkingDirectory(buffer, FILENAME_MAX);
    return std::string(buffer);
}

void CodeManager::addSourceFile(llvm::StringRef sourceFilePath) {
    auto currentDirectory = currentDirectoryStack.back();
    auto absoluteFilePath = std::string(currentDirectory).append("/").append(sourceFilePath);

    if (!llvm::sys::fs::exists(absoluteFilePath)) {
        diag.report(DIAG_ERROR, "File at path '{0}' doesn't exist", sourceFilePath);
        return;
    }

    for (auto filePath : sourceFilePaths) {
        if (filePath == absoluteFilePath) {
            return diag.report(DIAG_ERROR, "Cannot load source file at path '{0}' twice", sourceFilePath);
        }
    }

    sourceFilePaths.push_back(absoluteFilePath);
}

void CodeManager::addSourceText(llvm::StringRef sourceText) {
    sourceContents.push_back(sourceText);
}

void CodeManager::loadPendingSourceFiles() {
    while (bufferFilePathIndex < sourceFilePaths.size()) {
        auto filePath = sourceFilePaths[bufferFilePathIndex];
        {
            std::fstream file;
            file.open(filePath, std::fstream::in);

            if (file.is_open() && file.good()) {
                std::string content = { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
                sourceContents.push_back(content);
            } else {
                diag.report(DIAG_ERROR, "Couldn't load contents of file at path '{0}'", filePath);
            }

            file.close();
        }
        bufferFilePathIndex += 1;
    }
}

void CodeManager::parseAST() {
    loadPendingSourceFiles();

    while (parseFileIndex < sourceContents.size()) {
        auto content = sourceContents[parseFileIndex];
        {
            Lexer lexer(content.c_str());
            Parser parser(&lexer, &context, &diag);
            parser.parseAllTopLevelNodes();

            auto module = context.getModule();
            for (auto it = module->declsBegin() + preprocessDeclIndex; it != module->declsEnd(); it++) {
                if ((*it)->kind == AST_LOAD_DIRECTIVE) {
                    auto loadDirective = reinterpret_cast<ASTLoadDirective*>(*it);
                    addSourceFile(loadDirective->loadFilePath);
                    if (diag.hasErrors()) {
                        return;
                    }
                }

                preprocessDeclIndex += 1;
            }
        }
        parseFileIndex += 1;

        loadPendingSourceFiles();
    }
}

void CodeManager::typecheckAST() {
    parseAST();
    if (diag.hasErrors()) { return; }

    Sema sema(this);
    sema.validateAST();
}
