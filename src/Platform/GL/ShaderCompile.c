#include "Platform/GL/ShaderCompile.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(SDL2_RENDER) && !defined(OPENJKDF2_SHADER_PURE_TEST)
#include "SDL2_helper.h"
#include "General/DiagnosticLog.h"
#endif

typedef struct ShaderSha256
{
    uint32_t state[8];
    uint64_t bit_count;
    unsigned char block[64];
    size_t block_size;
} ShaderSha256;

static const uint32_t shader_sha256_constants[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

static uint32_t shader_rotate_right(uint32_t value, unsigned count)
{
    return (value >> count) | (value << (32 - count));
}

static void shader_sha256_transform(ShaderSha256* context)
{
    uint32_t words[64];
    uint32_t a, b, c, d, e, f, g, h;
    size_t index;
    for (index = 0; index < 16; ++index) {
        size_t offset = index * 4;
        words[index] = ((uint32_t)context->block[offset] << 24) |
                       ((uint32_t)context->block[offset + 1] << 16) |
                       ((uint32_t)context->block[offset + 2] << 8) |
                       context->block[offset + 3];
    }
    for (index = 16; index < 64; ++index) {
        uint32_t s0 = shader_rotate_right(words[index - 15], 7) ^ shader_rotate_right(words[index - 15], 18) ^ (words[index - 15] >> 3);
        uint32_t s1 = shader_rotate_right(words[index - 2], 17) ^ shader_rotate_right(words[index - 2], 19) ^ (words[index - 2] >> 10);
        words[index] = words[index - 16] + s0 + words[index - 7] + s1;
    }
    a=context->state[0]; b=context->state[1]; c=context->state[2]; d=context->state[3];
    e=context->state[4]; f=context->state[5]; g=context->state[6]; h=context->state[7];
    for (index = 0; index < 64; ++index) {
        uint32_t sum1 = shader_rotate_right(e,6) ^ shader_rotate_right(e,11) ^ shader_rotate_right(e,25);
        uint32_t choice = (e & f) ^ ((~e) & g);
        uint32_t temporary1 = h + sum1 + choice + shader_sha256_constants[index] + words[index];
        uint32_t sum0 = shader_rotate_right(a,2) ^ shader_rotate_right(a,13) ^ shader_rotate_right(a,22);
        uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temporary2 = sum0 + majority;
        h=g; g=f; f=e; e=d+temporary1; d=c; c=b; b=a; a=temporary1+temporary2;
    }
    context->state[0]+=a; context->state[1]+=b; context->state[2]+=c; context->state[3]+=d;
    context->state[4]+=e; context->state[5]+=f; context->state[6]+=g; context->state[7]+=h;
}

static void shader_sha256_init(ShaderSha256* context)
{
    static const uint32_t initial[8] = { 0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19 };
    memcpy(context->state, initial, sizeof(initial));
    context->bit_count = 0;
    context->block_size = 0;
}

static void shader_sha256_update(ShaderSha256* context, const unsigned char* data, size_t size)
{
    while (size--) {
        context->block[context->block_size++] = *data++;
        context->bit_count += 8;
        if (context->block_size == 64) {
            shader_sha256_transform(context);
            context->block_size = 0;
        }
    }
}

static void shader_sha256_finish(ShaderSha256* context, unsigned char digest[32])
{
    uint64_t original_bits = context->bit_count;
    size_t index;
    context->block[context->block_size++] = 0x80;
    if (context->block_size > 56) {
        while (context->block_size < 64) context->block[context->block_size++] = 0;
        shader_sha256_transform(context);
        context->block_size = 0;
    }
    while (context->block_size < 56) context->block[context->block_size++] = 0;
    for (index = 0; index < 8; ++index) context->block[63 - index] = (unsigned char)(original_bits >> (index * 8));
    shader_sha256_transform(context);
    for (index = 0; index < 32; ++index) digest[index] = (unsigned char)(context->state[index / 4] >> (24 - (index % 4) * 8));
}

const char* shader_stage_name(ShaderStage stage)
{
    switch (stage) {
        case SHADER_STAGE_VERTEX: return "vertex";
        case SHADER_STAGE_FRAGMENT: return "fragment";
        case SHADER_STAGE_PROGRAM: return "program";
        default: return "unknown";
    }
}

void shader_source_sha256(const char* source, char output[65])
{
    static const char hex[] = "0123456789abcdef";
    ShaderSha256 context;
    unsigned char digest[32];
    size_t index;
    shader_sha256_init(&context);
    if (source) shader_sha256_update(&context, (const unsigned char*)source, strlen(source));
    shader_sha256_finish(&context, digest);
    for (index = 0; index < 32; ++index) {
        output[index * 2] = hex[digest[index] >> 4];
        output[index * 2 + 1] = hex[digest[index] & 15];
    }
    output[64] = '\0';
}

bool shader_normalize_log(const char* input, char* output, size_t output_size)
{
    static const char suffix[] = "[truncated]";
    size_t input_length = input ? strlen(input) : 0;
    size_t suffix_length = sizeof(suffix) - 1;
    size_t limit;
    size_t read_index = 0;
    size_t write_index = 0;
    bool truncated = false;
    if (!output || !output_size) return input_length != 0;
    limit = output_size - 1;
    while (read_index < input_length && write_index < limit) {
        unsigned char value = (unsigned char)input[read_index++];
        if (value == '\r' && read_index < input_length && input[read_index] == '\n') ++read_index;
        if (value < 0x20 || value == 0x7f) value = ' ';
        output[write_index++] = (char)value;
    }
    truncated = read_index < input_length;
    if (truncated && limit >= suffix_length) {
        write_index = limit - suffix_length;
        memcpy(output + write_index, suffix, suffix_length);
        write_index += suffix_length;
    }
    output[write_index] = '\0';
    return truncated;
}

#if defined(SDL2_RENDER) && !defined(OPENJKDF2_SHADER_PURE_TEST)
static void shader_result_init(ShaderCompileResult* result, const char* name, ShaderStage stage, const char* source)
{
    memset(result, 0, sizeof(*result));
    result->stage = stage;
    snprintf(result->name, sizeof(result->name), "%s", name ? name : "unnamed");
    shader_source_sha256(source, result->source_sha256);
}

static void shader_emit_result(const ShaderCompileResult* result, const char* event_name)
{
    char event[4608];
    snprintf(event, sizeof(event), "%s name=%s stage=%s hash=%s ok=%s log=%s",
             event_name, result->name, shader_stage_name(result->stage), result->source_sha256,
             result->ok ? "true" : "false", result->log[0] ? result->log : "none");
    diag_log_event(result->ok ? DIAG_SEVERITY_INFO : DIAG_SEVERITY_ERROR, "renderer", event);
}

static void shader_capture_log(unsigned object, bool program, char output[4096])
{
    GLint requested = 0;
    GLsizei written = 0;
    char* raw;
    if (program) glGetProgramiv(object, GL_INFO_LOG_LENGTH, &requested);
    else glGetShaderiv(object, GL_INFO_LOG_LENGTH, &requested);
    if (requested <= 1) {
        output[0] = '\0';
        return;
    }
    if (requested > 65536) requested = 65536;
    raw = (char*)calloc((size_t)requested + 1, 1);
    if (!raw) {
        snprintf(output, 4096, "compiler log allocation failed");
        return;
    }
    if (program) glGetProgramInfoLog(object, requested, &written, raw);
    else glGetShaderInfoLog(object, requested, &written, raw);
    raw[written >= 0 && written <= requested ? written : requested] = '\0';
    shader_normalize_log(raw, output, 4096);
    free(raw);
}

ShaderCompileResult shader_compile_named(const char* name, ShaderStage stage, const char* source)
{
    ShaderCompileResult result;
    GLenum gl_stage;
    GLint compile_ok = GL_FALSE;
    const GLchar* sources[5];
    const char* version = "#version 330\n";
    const char* extensions = "#extension GL_ARB_texture_gather : enable\n";
    const char* defines = "#define CAN_BILINEAR_FILTER\n#define HAS_MIPS\n";
    const char* precision =
        "#ifdef GL_ES\n#ifdef GL_FRAGMENT_PRECISION_HIGH\nprecision highp float;\n"
        "#else\nprecision mediump float;\n#endif\n#else\n#define lowp\n#define mediump\n#define highp\n#endif\n";

    shader_result_init(&result, name, stage, source ? source : "");
    gl_stage = stage == SHADER_STAGE_VERTEX ? GL_VERTEX_SHADER :
               stage == SHADER_STAGE_FRAGMENT ? GL_FRAGMENT_SHADER : 0;
    if (!gl_stage || !source) {
        snprintf(result.log, sizeof(result.log), "invalid shader stage or source");
        shader_emit_result(&result, "shader_compile");
        return result;
    }
#if defined(ARCH_WASM) || defined(TARGET_ANDROID)
    version = "#version 300 es\n";
    extensions = "\n";
    defines = "#define CAN_BILINEAR_FILTER\n";
#endif
    result.object = glCreateShader(gl_stage);
    if (!result.object) {
        snprintf(result.log, sizeof(result.log), "glCreateShader returned zero");
        shader_emit_result(&result, "shader_compile");
        return result;
    }
    sources[0] = version; sources[1] = extensions; sources[2] = defines; sources[3] = precision; sources[4] = source;
    glShaderSource(result.object, 5, sources, NULL);
    glCompileShader(result.object);
    glGetShaderiv(result.object, GL_COMPILE_STATUS, &compile_ok);
    shader_capture_log(result.object, false, result.log);
    result.ok = compile_ok == GL_TRUE;
    if (!result.ok) {
        glDeleteShader(result.object);
        result.object = 0;
    }
    shader_emit_result(&result, "shader_compile");
    return result;
}

ShaderCompileResult shader_link_named(const char* name, unsigned int vertex_shader, unsigned int fragment_shader)
{
    ShaderCompileResult result;
    GLint link_ok = GL_FALSE;
    shader_result_init(&result, name, SHADER_STAGE_PROGRAM, "");
    result.object = glCreateProgram();
    if (result.object) {
        glAttachShader(result.object, vertex_shader);
        glAttachShader(result.object, fragment_shader);
        glLinkProgram(result.object);
        glGetProgramiv(result.object, GL_LINK_STATUS, &link_ok);
        shader_capture_log(result.object, true, result.log);
        result.ok = link_ok == GL_TRUE;
        glDetachShader(result.object, vertex_shader);
        glDetachShader(result.object, fragment_shader);
        if (!result.ok) {
            glDeleteProgram(result.object);
            result.object = 0;
        }
    } else {
        snprintf(result.log, sizeof(result.log), "glCreateProgram returned zero");
    }
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    shader_emit_result(&result, "shader_link");
    return result;
}
#endif
