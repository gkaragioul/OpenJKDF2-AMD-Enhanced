#include "General/DiagnosticLog.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#include <direct.h>
#define diag_mkdir(path) _mkdir(path)
#else
#include <sys/stat.h>
#define diag_mkdir(path) mkdir(path, 0700)
#endif

typedef struct DiagLogState
{
    FILE* text;
    FILE* jsonl;
    char directory[512];
    char data_dir[512];
    bool active;
    bool previous_run_unclean;
} DiagLogState;

static DiagLogState diag_state;

static bool diag_char_equal(char left, char right);

static bool diag_path_join(char* output, size_t size, const char* directory, const char* filename)
{
    int written;
    if (!output || !size || !directory || !filename) return false;
    written = snprintf(output, size, "%s/%s", directory, filename);
    return written >= 0 && (size_t)written < size;
}

static const char* diag_severity_name(DiagSeverity severity)
{
    switch (severity) {
        case DIAG_SEVERITY_DEBUG: return "debug";
        case DIAG_SEVERITY_INFO: return "info";
        case DIAG_SEVERITY_WARNING: return "warning";
        case DIAG_SEVERITY_ERROR: return "error";
        default: return "unknown";
    }
}

static void diag_timestamp(char output[32])
{
    time_t now = time(NULL);
    struct tm utc;
#if defined(_WIN32)
    gmtime_s(&utc, &now);
#else
    gmtime_r(&now, &utc);
#endif
    strftime(output, 32, "%Y-%m-%dT%H:%M:%SZ", &utc);
}

static void diag_write_json_string(FILE* stream, const char* text)
{
    const unsigned char* cursor = (const unsigned char*)(text ? text : "");
    fputc('"', stream);
    while (*cursor) {
        switch (*cursor) {
            case '"': fputs("\\\"", stream); break;
            case '\\': fputs("\\\\", stream); break;
            case '\b': fputs("\\b", stream); break;
            case '\f': fputs("\\f", stream); break;
            case '\n': fputs("\\n", stream); break;
            case '\r': fputs("\\r", stream); break;
            case '\t': fputs("\\t", stream); break;
            default:
                if (*cursor < 0x20) fprintf(stream, "\\u%04x", *cursor);
                else fputc(*cursor, stream);
                break;
        }
        ++cursor;
    }
    fputc('"', stream);
}

static bool diag_write_run_state(const char* status)
{
    char path[640];
    char temporary[640];
    FILE* stream;
    if (!diag_path_join(path, sizeof(path), diag_state.directory, "run-state.json") ||
        !diag_path_join(temporary, sizeof(temporary), diag_state.directory, "run-state.tmp")) return false;
    stream = fopen(temporary, "wb");
    if (!stream) return false;
    fprintf(stream, "{\"schema\":1,\"status\":\"%s\"}\n", status);
    if (fclose(stream) != 0) return false;
    remove(path);
    return rename(temporary, path) == 0;
}

static bool diag_read_previous_run_unclean(const char* directory)
{
    char path[640];
    char contents[128];
    size_t count;
    FILE* stream;
    if (!diag_path_join(path, sizeof(path), directory, "run-state.json")) return false;
    stream = fopen(path, "rb");
    if (!stream) return false;
    count = fread(contents, 1, sizeof(contents) - 1, stream);
    fclose(stream);
    contents[count] = '\0';
    return strstr(contents, "\"status\":\"unclean\"") != NULL;
}

bool diag_log_start(const DiagLogConfig* config)
{
    char text_path[640];
    char jsonl_path[640];
    if (!config || !config->diagnostics_dir || !config->diagnostics_dir[0] || diag_state.active) return false;
    if (strlen(config->diagnostics_dir) >= sizeof(diag_state.directory)) return false;
    strcpy(diag_state.directory, config->diagnostics_dir);
    diag_state.previous_run_unclean = diag_read_previous_run_unclean(diag_state.directory);
    if (config->data_dir) {
        if (strlen(config->data_dir) >= sizeof(diag_state.data_dir)) return false;
        strcpy(diag_state.data_dir, config->data_dir);
    }
    diag_mkdir(diag_state.directory);
    if (!diag_path_join(text_path, sizeof(text_path), diag_state.directory, "openjkdf2.log") ||
        !diag_path_join(jsonl_path, sizeof(jsonl_path), diag_state.directory, "openjkdf2.jsonl")) return false;
    diag_state.text = fopen(text_path, "wb");
    diag_state.jsonl = fopen(jsonl_path, "wb");
    if (!diag_state.text || !diag_state.jsonl) {
        if (diag_state.text) fclose(diag_state.text);
        if (diag_state.jsonl) fclose(diag_state.jsonl);
        memset(&diag_state, 0, sizeof(diag_state));
        return false;
    }
    diag_state.active = true;
    if (!diag_write_run_state("unclean")) {
        diag_log_finish(false);
        return false;
    }
    return true;
}

bool diag_log_previous_run_unclean(void)
{
    return diag_state.active && diag_state.previous_run_unclean;
}

bool diag_log_event(DiagSeverity severity, const char* subsystem, const char* event)
{
    char timestamp[32];
    char safe_event[4096];
    const char* severity_name;
    size_t input_index = 0;
    size_t output_index = 0;
    size_t root_length = strlen(diag_state.data_dir);
    if (!diag_state.active || !subsystem || !event) return false;
    while (event[input_index] && output_index + 1 < sizeof(safe_event)) {
        size_t match_index = 0;
        while (root_length && match_index < root_length && event[input_index + match_index] &&
               diag_char_equal(event[input_index + match_index], diag_state.data_dir[match_index])) {
            ++match_index;
        }
        if (root_length && match_index == root_length) {
            static const char replacement[] = "<data-dir>";
            size_t replacement_length = sizeof(replacement) - 1;
            if (output_index + replacement_length >= sizeof(safe_event)) break;
            memcpy(safe_event + output_index, replacement, replacement_length);
            output_index += replacement_length;
            input_index += root_length;
        } else {
            safe_event[output_index++] = event[input_index++];
        }
    }
    safe_event[output_index] = '\0';
    diag_timestamp(timestamp);
    severity_name = diag_severity_name(severity);
    fprintf(diag_state.text, "%s [%s] %s: %s\n", timestamp, severity_name, subsystem, safe_event);
    fputs("{\"schema\":1,\"timestamp_utc\":", diag_state.jsonl);
    diag_write_json_string(diag_state.jsonl, timestamp);
    fputs(",\"severity\":", diag_state.jsonl);
    diag_write_json_string(diag_state.jsonl, severity_name);
    fputs(",\"subsystem\":", diag_state.jsonl);
    diag_write_json_string(diag_state.jsonl, subsystem);
    fputs(",\"event\":", diag_state.jsonl);
    diag_write_json_string(diag_state.jsonl, safe_event);
    fputs(",\"fields\":{}}\n", diag_state.jsonl);
    return fflush(diag_state.text) == 0 && fflush(diag_state.jsonl) == 0;
}

bool diag_log_finish(bool clean)
{
    bool result;
    if (!diag_state.active) return false;
    result = fclose(diag_state.text) == 0;
    result = fclose(diag_state.jsonl) == 0 && result;
    diag_state.text = NULL;
    diag_state.jsonl = NULL;
    result = diag_write_run_state(clean ? "clean" : "unclean") && result;
    memset(&diag_state, 0, sizeof(diag_state));
    return result;
}

static bool diag_char_equal(char left, char right)
{
#if defined(_WIN32)
    if (left == '/') left = '\\';
    if (right == '/') right = '\\';
    return tolower((unsigned char)left) == tolower((unsigned char)right);
#else
    return left == right;
#endif
}

bool diag_redact_path(const char* path, const char* data_dir, char* output, size_t output_size)
{
    size_t root_length;
    size_t path_length;
    size_t index;
    const char* suffix;
    int written;
    if (!path || !data_dir || !output || !output_size) return false;
    root_length = strlen(data_dir);
    path_length = strlen(path);
    if (!root_length || path_length < root_length) return false;
    for (index = 0; index < root_length; ++index) {
        if (!diag_char_equal(path[index], data_dir[index])) return false;
    }
    if (path_length > root_length && path[root_length] != '/' && path[root_length] != '\\') return false;
    suffix = path + root_length;
    written = snprintf(output, output_size, "<data-dir>%s", suffix);
    return written >= 0 && (size_t)written < output_size;
}
