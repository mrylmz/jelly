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

#include "Basic/DiagnosticEngine.h"
#include "Basic/DiagnosticHandler.h"

using namespace jelly;

DiagnosticEngine::DiagnosticEngine(DiagnosticHandler* handler) :
handler(handler) {

}

DiagnosticEngine::~DiagnosticEngine() {
    handler->finish();
}

uint64_t DiagnosticEngine::getReportedInfoCount() const {
    return reportedDiagnosticCounts[(uint8_t)Diagnostic::Level::Info];
}

uint64_t DiagnosticEngine::getReportedWarningCount() const {
    return reportedDiagnosticCounts[(uint8_t)Diagnostic::Level::Warning];
}

uint64_t DiagnosticEngine::getReportedErrorCount() const {
    return reportedDiagnosticCounts[(uint8_t)Diagnostic::Level::Error];
}

uint64_t DiagnosticEngine::getReportedFatalCount() const {
    return reportedDiagnosticCounts[(uint8_t)Diagnostic::Level::Fatal];
}

void DiagnosticEngine::begin(SourceBuffer buffer) {
    if (currentBuffer.isValid()) {
        handler->end();
    }

    currentBuffer = buffer;
    handler->begin(currentBuffer);
}

void DiagnosticEngine::report(Diagnostic::Level level, std::string message) {
    Diagnostic diagnostic(level, std::move(message));

    reportedDiagnosticCounts[(uint8_t)level] += 1;
    handler->handle(diagnostic);
}
