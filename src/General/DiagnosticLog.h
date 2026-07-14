#ifndef OPENJKDF2_DIAGNOSTIC_LOG_H
#define OPENJKDF2_DIAGNOSTIC_LOG_H

#include <stdbool.h>
#include <stddef.h>

typedef enum DiagSeverity
{
    DIAG_SEVERITY_DEBUG,
    DIAG_SEVERITY_INFO,
    DIAG_SEVERITY_WARNING,
    DIAG_SEVERITY_ERROR
} DiagSeverity;

typedef struct DiagLogConfig
{
    const char* diagnostics_dir;
    const char* data_dir;
} DiagLogConfig;

bool diag_log_start(const DiagLogConfig* config);
bool diag_log_previous_run_unclean(void);
bool diag_log_event(DiagSeverity severity, const char* subsystem, const char* event);
bool diag_log_finish(bool clean);
bool diag_redact_path(const char* path, const char* data_dir, char* output, size_t output_size);

#endif
