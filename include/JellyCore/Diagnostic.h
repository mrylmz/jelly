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

void DiagnosticEngineResetMessageCounts();

Index DiagnosticEngineGetMessageCount(DiagnosticLevel level);

void ReportDebug(const Char *message);
void ReportDebugString(StringRef message);
void ReportDebugFormat(const Char *format, ...) JELLY_PRINTFLIKE(1, 2);

void ReportInfo(const Char *message);
void ReportInfoString(StringRef message);
void ReportInfoFormat(const Char *format, ...) JELLY_PRINTFLIKE(1, 2);

void ReportWarning(const Char *message);
void ReportWarningString(StringRef message);
void ReportWarningFormat(const Char *format, ...) JELLY_PRINTFLIKE(1, 2);

void ReportError(const Char *message);
void ReportErrorString(StringRef message);
void ReportErrorFormat(const Char *format, ...) JELLY_PRINTFLIKE(1, 2);

void ReportCritical(const Char *message);
void ReportCriticalString(StringRef message);
void ReportCriticalFormat(const Char *format, ...) JELLY_PRINTFLIKE(1, 2);

void FatalError(const Char *message);

JELLY_EXTERN_C_END

#endif
