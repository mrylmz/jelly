#include "JellyCore/Diagnostic.h"

struct _DiagnosticEngine {
    DiagnosticHandler handler;
    void *context;
};

void _DiagnosticHandlerStd(DiagnosticLevel level, const Char *message, void *context);

void _ReportDiagnostic(DiagnosticLevel level, const Char *message);

static struct _DiagnosticEngine kSharedDiagnosticEngine = { &_DiagnosticHandlerStd, NULL };

void DiagnosticEngineSetDefaultHandler(DiagnosticHandler handler, void *context) {
    kSharedDiagnosticEngine.handler = handler;
    kSharedDiagnosticEngine.context = context;
}

void ReportDebug(const Char *message) {
    _ReportDiagnostic(DiagnosticLevelDebug, message);
}

void ReportDebugString(StringRef message) {
    _ReportDiagnostic(DiagnosticLevelDebug, StringGetCharacters(message));
}

void ReportInfo(const Char *message) {
    _ReportDiagnostic(DiagnosticLevelInfo, message);
}

void ReportInfoString(StringRef message) {
    _ReportDiagnostic(DiagnosticLevelInfo, StringGetCharacters(message));
}

void ReportWarning(const Char *message) {
    _ReportDiagnostic(DiagnosticLevelWarning, message);
}

void ReportWarningString(StringRef message) {
    _ReportDiagnostic(DiagnosticLevelWarning, StringGetCharacters(message));
}

void ReportError(const Char *message) {
    _ReportDiagnostic(DiagnosticLevelError, message);
}

void ReportErrorString(StringRef message) {
    _ReportDiagnostic(DiagnosticLevelError, StringGetCharacters(message));
}

void ReportCritical(const Char *message) {
    _ReportDiagnostic(DiagnosticLevelCritical, message);
}

void ReportCriticalString(StringRef message) {
    _ReportDiagnostic(DiagnosticLevelCritical, StringGetCharacters(message));
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
}

void FatalError(const Char *message) {
    ReportCritical("Fatal error encountered during compilation!");
    ReportInfo(message);
    exit(EXIT_FAILURE);
}

