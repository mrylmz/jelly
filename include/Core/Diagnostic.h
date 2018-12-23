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
#include <llvm/Support/Format.h>
#include <llvm/Support/FormatVariadic.h>

// @Refactor may remove DiagnosticEngine entirely and move API to CodeManager

enum DiagnosticLevel : uint8_t {
    DIAG_INFO,
    DIAG_WARNING,
    DIAG_ERROR,

    DIAG_LEVEL_COUNT
};

struct Diagnostic {
    DiagnosticLevel level;
    std::string message;
};

struct DiagnosticHandler {
    virtual void start() = 0;
    virtual void finish() = 0;
    virtual void handle(Diagnostic diagnostic) = 0;
};

struct DiagnosticEngine {
    DiagnosticHandler* handler;
    uint64_t reportedCount[DIAG_LEVEL_COUNT];

    DiagnosticEngine(DiagnosticHandler* handler);
    ~DiagnosticEngine();

    bool hasErrors() const;

    unsigned getReportedCount(DiagnosticLevel level) const;

    template<typename ...Args>
    void report(DiagnosticLevel level, const char* format, Args&&... args) {
        Diagnostic diagnostic = {
            level,
            llvm::formatv(format, args...)
        };
        reportedCount[level] += 1;
        handler->handle(diagnostic);
    }
};
