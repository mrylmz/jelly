#ifndef __JELLY_DIAGNOSTIC__
#define __JELLY_DIAGNOSTIC__

#include <JellyCore/Base.h>
#include <JellyCore/String.h>

JELLY_EXTERN_C_BEGIN

enum _DiagnosticLevel {
    DiagnosticLevelDebug,
    DiagnosticLevelInfo,
    DiagnosticLevelWarning,
    DiagnosticLevelError,
    DiagnosticLevelCritical,

    DIAGNOSTIC_LEVEL_COUNT,
};
typedef enum _DiagnosticLevel DiagnosticLevel;

typedef void (*DiagnosticHandler)(DiagnosticLevel level, const Char *message, void *context);

void DiagnosticEngineSetDefaultHandler(DiagnosticHandler handler, void *context);

void ReportDebug(const Char *message);
void ReportDebugString(StringRef message);

void ReportInfo(const Char *message);
void ReportInfoString(StringRef message);

void ReportWarning(const Char *message);
void ReportWarningString(StringRef message);

void ReportError(const Char *message);
void ReportErrorString(StringRef message);

void ReportCritical(const Char *message);
void ReportCriticalString(StringRef message);

void FatalError(const Char *message);

JELLY_EXTERN_C_END

#endif
