//
// MIT License
//
// Copyright (c) 2019 Murat Yilmaz
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

#include "Basic/Diagnostic.h"
#include "Basic/LLVM.h"

#include <stdint.h>
#include <string>

// @Todo Add reporting diagnostics based on active SourceBuffer!

namespace jelly {

    class DiagnosticHandler;

    class DiagnosticEngine {
        DiagnosticHandler* handler;
        uint64_t reportedDiagnosticCounts[4] = { };

    public:

        DiagnosticEngine(DiagnosticHandler* handler);
        ~DiagnosticEngine();

        uint64_t getReportedInfoCount() const;
        uint64_t getReportedWarningCount() const;
        uint64_t getReportedErrorCount() const;
        uint64_t getReportedFatalCount() const;

        template<typename ...Args>
        void report(Diagnostic::Level level, const char* format, Args&&... args) {
            std::string message = formatv(format, args...);

            report(level, std::move(message));
        }

        void report(Diagnostic::Level level, std::string message);
    };
}
