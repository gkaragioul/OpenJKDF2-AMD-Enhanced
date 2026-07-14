#include "Platform/GL/ShaderCompile.h"

#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(value) do { if (!(value)) { fprintf(stderr, "line %d: %s\n", __LINE__, #value); ++failures; } } while (0)

int main(void)
{
    char hash[65];
    char normalized[24];
    bool truncated;

    CHECK(strcmp(shader_stage_name(SHADER_STAGE_VERTEX), "vertex") == 0);
    CHECK(strcmp(shader_stage_name(SHADER_STAGE_FRAGMENT), "fragment") == 0);
    CHECK(strcmp(shader_stage_name(SHADER_STAGE_PROGRAM), "program") == 0);
    CHECK(strcmp(shader_stage_name((ShaderStage)99), "unknown") == 0);

    shader_source_sha256("abc", hash);
    CHECK(strcmp(hash, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") == 0);

    truncated = shader_normalize_log("bad\r\nline\t\x01" "abcdefghijklmnopqrstuvwxyz", normalized, sizeof(normalized));
    CHECK(truncated);
    CHECK(strchr(normalized, '\r') == NULL);
    CHECK(strchr(normalized, '\n') == NULL);
    CHECK(strchr(normalized, '\t') == NULL);
    CHECK(strstr(normalized, "[truncated]") != NULL);

    return failures ? 1 : 0;
}
