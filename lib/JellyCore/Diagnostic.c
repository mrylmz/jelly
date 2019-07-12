#include "JellyCore/Diagnostic.h"

struct _DiagnosticEngine {
    DiagnosticHandler handler;
    void *context;
    Char formatBuffer[65535];
    Index messageCount[DIAGNOSTIC_LEVEL_COUNT];
};

void _DiagnosticHandlerStd(DiagnosticLevel level, const Char *message, void *context);

void _ReportDiagnostic(DiagnosticLevel level, const Char *message);

static struct _DiagnosticEngine kSharedDiagnosticEngine = {&_DiagnosticHandlerStd, NULL};

void DiagnosticEngineSetDefaultHandler(DiagnosticHandler handler, void *context) {
    if (handler) {
        kSharedDiagnosticEngine.handler = handler;
    } else {
        kSharedDiagnosticEngine.handler = &_DiagnosticHandlerStd;
    }

    kSharedDiagnosticEngine.context = context;
}

void DiagnosticEngineResetMessageCounts() {
    for (Index level = 0; level < DIAGNOSTIC_LEVEL_COUNT; level++) {
        kSharedDiagnosticEngine.messageCount[level] = 0;
    }
}

Index DiagnosticEngineGetMessageCount(DiagnosticLevel level) {
    return kSharedDiagnosticEngine.messageCount[level];
}

void ReportDebug(const Char *message) {
    _ReportDiagnostic(DiagnosticLevelDebug, message);
}

void ReportDebugString(StringRef message) {
    _ReportDiagnostic(DiagnosticLevelDebug, StringGetCharacters(message));
}

void ReportDebugFormat(const Char *format, ...) {
    va_list argumentPointer;
    va_start(argumentPointer, format);
    vsprintf(&kSharedDiagnosticEngine.formatBuffer[0], format, argumentPointer);
    va_end(argumentPointer);

    ReportError(&kSharedDiagnosticEngine.formatBuffer[0]);
}

void ReportInfo(const Char *message) {
    _ReportDiagnostic(DiagnosticLevelInfo, message);
}

void ReportInfoString(StringRef message) {
    _ReportDiagnostic(DiagnosticLevelInfo, StringGetCharacters(message));
}

void ReportInfoFormat(const Char *format, ...) {
    va_list argumentPointer;
    va_start(argumentPointer, format);
    vsprintf(&kSharedDiagnosticEngine.formatBuffer[0], format, argumentPointer);
    va_end(argumentPointer);

    _ReportDiagnostic(DiagnosticLevelInfo, &kSharedDiagnosticEngine.formatBuffer[0]);
}

void ReportWarning(const Char *message) {
    _ReportDiagnostic(DiagnosticLevelWarning, message);
}

void ReportWarningString(StringRef message) {
    _ReportDiagnostic(DiagnosticLevelWarning, StringGetCharacters(message));
}

void ReportWarningFormat(const Char *format, ...) {
    va_list argumentPointer;
    va_start(argumentPointer, format);
    vsprintf(&kSharedDiagnosticEngine.formatBuffer[0], format, argumentPointer);
    va_end(argumentPointer);

    _ReportDiagnostic(DiagnosticLevelWarning, &kSharedDiagnosticEngine.formatBuffer[0]);
}

void ReportError(const Char *message) {
    _ReportDiagnostic(DiagnosticLevelError, message);
}

void ReportErrorString(StringRef message) {
    _ReportDiagnostic(DiagnosticLevelError, StringGetCharacters(message));
}

void ReportErrorFormat(const Char *format, ...) {
    va_list argumentPointer;
    va_start(argumentPointer, format);
    vsprintf(kSharedDiagnosticEngine.formatBuffer, format, argumentPointer);
    va_end(argumentPointer);

    _ReportDiagnostic(DiagnosticLevelError, &kSharedDiagnosticEngine.formatBuffer[0]);
}

void ReportCritical(const Char *message) {
    _ReportDiagnostic(DiagnosticLevelCritical, message);
}

void ReportCriticalString(StringRef message) {
    _ReportDiagnostic(DiagnosticLevelCritical, StringGetCharacters(message));
}

void ReportCriticalFormat(const Char *format, ...) {
    va_list argumentPointer;
    va_start(argumentPointer, format);
    vsprintf(&kSharedDiagnosticEngine.formatBuffer[0], format, argumentPointer);
    va_end(argumentPointer);

    _ReportDiagnostic(DiagnosticLevelCritical, &kSharedDiagnosticEngine.formatBuffer[0]);
}

void _DiagnosticHandlerStd(DiagnosticLevel level, const Char *message, void *context) {
    switch (level) {
    case DiagnosticLevelDebug:
        fprintf(stdout, "[DEBUG] %s\n", message);
        break;

    case DiagnosticLevelInfo:
        fprintf(stdout, "[INFO] %s\n", message);
        break;

    case DiagnosticLevelWarning:
        fprintf(stdout, "[WARNING] %s\n", message);
        break;

    case DiagnosticLevelError:
        fprintf(stderr, "[ERROR] %s\n", message);
        break;

    case DiagnosticLevelCritical:
        fprintf(stderr, "[CRITICAL] %s\n", message);
        break;

    default:
        JELLY_UNREACHABLE("Unknown diagnostic level!");
    }
}

void _ReportDiagnostic(DiagnosticLevel level, const Char *message) {
    assert(kSharedDiagnosticEngine.handler);
    kSharedDiagnosticEngine.handler(level, message, kSharedDiagnosticEngine.context);
    kSharedDiagnosticEngine.messageCount[level] += 1;
}

void FatalError(const Char *message) {
    ReportCritical("Fatal error encountered during compilation!");
    ReportInfo(message);
    exit(EXIT_FAILURE);
}
