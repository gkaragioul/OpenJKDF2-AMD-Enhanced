#include "General/DiagnosticLog.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <direct.h>
#define make_dir(path) _mkdir(path)
#else
#include <sys/stat.h>
#define make_dir(path) mkdir(path, 0700)
#endif

static int failures;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #condition); \
        ++failures; \
    } \
} while (0)

static char* read_file(const char* path)
{
    FILE* stream = fopen(path, "rb");
    long length;
    char* text;
    if (!stream) return NULL;
    fseek(stream, 0, SEEK_END);
    length = ftell(stream);
    rewind(stream);
    text = (char*)malloc((size_t)length + 1);
    if (text) {
        fread(text, 1, (size_t)length, stream);
        text[length] = '\0';
    }
    fclose(stream);
    return text;
}

static void write_file(const char* path, const char* text)
{
    FILE* stream = fopen(path, "wb");
    CHECK(stream != NULL);
    if (stream) {
        fputs(text, stream);
        fclose(stream);
    }
}

int main(void)
{
    const char* output_dir = "test-output-diagnostic-log";
    DiagLogConfig config = { output_dir, "C:\\Users\\alice\\Games\\JK" };
    char redacted[256];
    char* text;

    make_dir(output_dir);
    write_file("test-output-diagnostic-log/run-state.json", "{\"schema\":1,\"status\":\"unclean\"}\n");
    CHECK(diag_redact_path(
        "C:\\Users\\alice\\Games\\JK\\episode\\JK1.gob",
        config.data_dir,
        redacted,
        sizeof(redacted)));
    CHECK(strcmp(redacted, "<data-dir>\\episode\\JK1.gob") == 0);

    CHECK(diag_log_start(&config));
    CHECK(diag_log_previous_run_unclean());
    text = read_file("test-output-diagnostic-log/run-state.json");
    CHECK(text != NULL && strstr(text, "\"status\":\"unclean\"") != NULL);
    free(text);

    CHECK(diag_log_event(
        DIAG_SEVERITY_WARNING,
        "renderer",
        "quote=\" newline=\n file=C:\\Users\\alice\\Games\\JK\\episode\\JK1.gob"));
    CHECK(diag_log_finish(true));

    text = read_file("test-output-diagnostic-log/openjkdf2.jsonl");
    CHECK(text != NULL);
    if (text) {
        CHECK(strstr(text, "\"schema\":1") != NULL);
        CHECK(strstr(text, "\"severity\":\"warning\"") != NULL);
        CHECK(strstr(text, "quote=\\\"") != NULL);
        CHECK(strstr(text, "newline=\\n") != NULL);
        CHECK(strstr(text, "<data-dir>\\\\episode\\\\JK1.gob") != NULL);
        CHECK(strstr(text, "alice") == NULL);
        CHECK(strchr(text, '\n') == strrchr(text, '\n'));
    }
    free(text);

    text = read_file("test-output-diagnostic-log/run-state.json");
    CHECK(text != NULL && strstr(text, "\"status\":\"clean\"") != NULL);
    free(text);

    CHECK(diag_log_start(&config));
    CHECK(!diag_log_previous_run_unclean());
    CHECK(diag_log_finish(true));

    return failures == 0 ? 0 : 1;
}
