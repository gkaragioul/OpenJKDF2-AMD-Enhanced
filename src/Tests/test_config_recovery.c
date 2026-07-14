#include "General/ConfigRecovery.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #condition); \
        ++failures; \
    } \
} while (0)

static void write_text(const char* path, const char* text)
{
    FILE* stream = fopen(path, "wb");
    CHECK(stream != NULL);
    if (stream) {
        fwrite(text, 1, strlen(text), stream);
        fclose(stream);
    }
}

static char* read_text(const char* path)
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

int main(void)
{
    const char* active = "test-config-recovery-active.json";
    const char* snapshot = "test-config-recovery-lkg.json";
    char* text;

    remove(active);
    remove(snapshot);
    remove("test-config-recovery-lkg.json.tmp");
    remove("test-config-recovery-active.json.tmp");

    CHECK(config_recovery_should_offer(true, true, false));
    CHECK(!config_recovery_should_offer(false, true, false));
    CHECK(!config_recovery_should_offer(true, false, false));
    CHECK(!config_recovery_should_offer(true, true, true));

    CHECK(!config_recovery_has_snapshot(snapshot));
    write_text(active, "{\"mode\":\"borderless\"}\n");
    CHECK(config_recovery_snapshot(active, snapshot));
    CHECK(config_recovery_has_snapshot(snapshot));

    write_text(active, "{\"mode\":\"exclusive\"}\n");
    CHECK(config_recovery_restore(snapshot, active));
    text = read_text(active);
    CHECK(text != NULL && strcmp(text, "{\"mode\":\"borderless\"}\n") == 0);
    free(text);

    remove(snapshot);
    CHECK(!config_recovery_restore(snapshot, active));
    text = read_text(active);
    CHECK(text != NULL && strcmp(text, "{\"mode\":\"borderless\"}\n") == 0);
    free(text);

    remove(active);
    return failures == 0 ? 0 : 1;
}
