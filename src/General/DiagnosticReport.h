#ifndef OPENJKDF2_DIAGNOSTIC_REPORT_H
#define OPENJKDF2_DIAGNOSTIC_REPORT_H

#include <stdbool.h>
#include <stddef.h>

typedef struct DiagnosticReport
{
    char build_version[64];
    char os_name[64];
    char os_version[64];
    char vendor[256];
    char renderer[256];
    char api_version[128];
    char shading_language_version[128];
    bool renderer_collected;
} DiagnosticReport;

bool diag_report_build(DiagnosticReport* report, const char* build_version, const char* os_name, const char* os_version);
bool diag_report_set_renderer(DiagnosticReport* report, const char* vendor, const char* renderer,
                              const char* api_version, const char* shading_language_version);
bool diag_report_to_json(const DiagnosticReport* report, char* output, size_t output_size);

#endif
