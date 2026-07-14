#include "General/DiagnosticReport.h"

#include <stdio.h>
#include <string.h>

static bool diag_report_copy(char* output, size_t capacity, const char* input)
{
    size_t length;
    if (!output || !capacity || !input) return false;
    length = strlen(input);
    if (length >= capacity) return false;
    memcpy(output, input, length + 1);
    return true;
}

static bool diag_report_escape(const char* input, char* output, size_t capacity)
{
    size_t read_index = 0;
    size_t write_index = 0;
    if (!input || !output || !capacity) return false;
    while (input[read_index]) {
        unsigned char value = (unsigned char)input[read_index++];
        const char* escape = NULL;
        if (value == '"') escape = "\\\"";
        else if (value == '\\') escape = "\\\\";
        else if (value == '\n') escape = "\\n";
        else if (value == '\r') escape = "\\r";
        else if (value == '\t') escape = "\\t";
        if (escape) {
            size_t length = strlen(escape);
            if (write_index + length >= capacity) return false;
            memcpy(output + write_index, escape, length);
            write_index += length;
        } else if (value < 0x20) {
            if (write_index + 6 >= capacity) return false;
            snprintf(output + write_index, capacity - write_index, "\\u%04x", value);
            write_index += 6;
        } else {
            if (write_index + 1 >= capacity) return false;
            output[write_index++] = (char)value;
        }
    }
    output[write_index] = '\0';
    return true;
}

bool diag_report_build(DiagnosticReport* report, const char* build_version, const char* os_name, const char* os_version)
{
    if (!report) return false;
    memset(report, 0, sizeof(*report));
    return diag_report_copy(report->build_version, sizeof(report->build_version), build_version ? build_version : "not_collected") &&
           diag_report_copy(report->os_name, sizeof(report->os_name), os_name ? os_name : "not_collected") &&
           diag_report_copy(report->os_version, sizeof(report->os_version), os_version ? os_version : "not_collected");
}

bool diag_report_set_renderer(DiagnosticReport* report, const char* vendor, const char* renderer,
                              const char* api_version, const char* shading_language_version)
{
    if (!report || !vendor || !renderer || !api_version || !shading_language_version) return false;
    if (!diag_report_copy(report->vendor, sizeof(report->vendor), vendor) ||
        !diag_report_copy(report->renderer, sizeof(report->renderer), renderer) ||
        !diag_report_copy(report->api_version, sizeof(report->api_version), api_version) ||
        !diag_report_copy(report->shading_language_version, sizeof(report->shading_language_version), shading_language_version)) return false;
    report->renderer_collected = true;
    return true;
}

bool diag_report_to_json(const DiagnosticReport* report, char* output, size_t output_size)
{
    char build[128], os_name[128], os_version[128], vendor[512], renderer[512], api[256], glsl[256];
    int written;
    const char* not_collected = "not_collected";
    if (!report || !output || !output_size) return false;
    if (!diag_report_escape(report->build_version, build, sizeof(build)) ||
        !diag_report_escape(report->os_name, os_name, sizeof(os_name)) ||
        !diag_report_escape(report->os_version, os_version, sizeof(os_version)) ||
        !diag_report_escape(report->renderer_collected ? report->vendor : not_collected, vendor, sizeof(vendor)) ||
        !diag_report_escape(report->renderer_collected ? report->renderer : not_collected, renderer, sizeof(renderer)) ||
        !diag_report_escape(report->renderer_collected ? report->api_version : not_collected, api, sizeof(api)) ||
        !diag_report_escape(report->renderer_collected ? report->shading_language_version : not_collected, glsl, sizeof(glsl))) return false;
    written = snprintf(output, output_size,
        "{\"schema\":1,\"build_version\":\"%s\",\"os\":{\"name\":\"%s\",\"version\":\"%s\"},"
        "\"renderer\":\"%s\",\"vendor\":\"%s\",\"api_version\":\"%s\",\"shading_language_version\":\"%s\"}",
        build, os_name, os_version, renderer, vendor, api, glsl);
    return written >= 0 && (size_t)written < output_size;
}
