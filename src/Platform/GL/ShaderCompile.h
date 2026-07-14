#ifndef OPENJKDF2_SHADER_COMPILE_H
#define OPENJKDF2_SHADER_COMPILE_H

#include <stdbool.h>
#include <stddef.h>

typedef enum ShaderStage
{
    SHADER_STAGE_VERTEX,
    SHADER_STAGE_FRAGMENT,
    SHADER_STAGE_PROGRAM
} ShaderStage;

typedef struct ShaderCompileResult
{
    bool ok;
    unsigned int object;
    ShaderStage stage;
    char name[64];
    char source_sha256[65];
    char log[4096];
} ShaderCompileResult;

const char* shader_stage_name(ShaderStage stage);
void shader_source_sha256(const char* source, char output[65]);
bool shader_normalize_log(const char* input, char* output, size_t output_size);

#if defined(SDL2_RENDER) && !defined(OPENJKDF2_SHADER_PURE_TEST)
ShaderCompileResult shader_compile_named(const char* name, ShaderStage stage, const char* source);
ShaderCompileResult shader_link_named(const char* name, unsigned int vertex_shader, unsigned int fragment_shader);
#endif

#endif
