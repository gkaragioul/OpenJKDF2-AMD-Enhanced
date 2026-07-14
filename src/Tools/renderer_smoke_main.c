#include "SDL2_helper.h"
#include "General/DiagnosticLog.h"
#include "General/DiagnosticReport.h"
#include "Platform/GL/ShaderCompile.h"

#include <stdio.h>
#include <string.h>

enum
{
    SMOKE_OK = 0,
    SMOKE_SDL_INIT = 2,
    SMOKE_CONTEXT = 3,
    SMOKE_LOADER = 4,
    SMOKE_SHADER = 5,
    SMOKE_FRAMEBUFFER = 6
};

static void smoke_log_gl_string(const char* label, GLenum name)
{
    const GLubyte* value = glGetString(name);
    char event[1024];
    snprintf(event, sizeof(event), "%s=%s", label, value ? (const char*)value : "not_collected");
    diag_log_event(DIAG_SEVERITY_INFO, "renderer-smoke", event);
}

static bool smoke_write_report(void)
{
    DiagnosticReport report;
    char json[2048];
    FILE* stream;
    const char* vendor = (const char*)glGetString(GL_VENDOR);
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    const char* version = (const char*)glGetString(GL_VERSION);
    const char* glsl = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    if (!diag_report_build(&report, OPENJKDF2_RELEASE_VERSION_STRING, "Windows", "not_collected") ||
        !diag_report_set_renderer(&report, vendor ? vendor : "not_collected", renderer ? renderer : "not_collected",
                                  version ? version : "not_collected", glsl ? glsl : "not_collected") ||
        !diag_report_to_json(&report, json, sizeof(json))) return false;
    stream = fopen("renderer-smoke-output/renderer-report.json", "wb");
    if (!stream) return false;
    fprintf(stream, "%s\n", json);
    return fclose(stream) == 0;
}

int main(int argc, char** argv)
{
    static const char vertex_source[] =
        "layout(location=0) in vec2 position;\n"
        "void main(){ gl_Position=vec4(position,0.0,1.0); }\n";
    static const char fragment_source[] =
        "out vec4 color;\n"
        "void main(){ color=vec4(0.2,0.4,0.6,1.0); }\n";
    static const GLfloat vertices[] = { -1.0f,-1.0f, 3.0f,-1.0f, -1.0f,3.0f };
    DiagLogConfig log_config = { "renderer-smoke-output", NULL };
    SDL_Window* window = NULL;
    SDL_GLContext context = NULL;
    ShaderCompileResult vertex = { 0 };
    ShaderCompileResult fragment = { 0 };
    ShaderCompileResult program = { 0 };
    GLuint vao = 0, vbo = 0, texture = 0, framebuffer = 0;
    unsigned char pixel[4] = { 0 };
    int result = SMOKE_OK;
    bool borderless = argc > 1 && strcmp(argv[1], "--borderless") == 0;
    int window_width = 64;
    int window_height = 64;
    SDL_WindowFlags window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN;

    diag_log_start(&log_config);
    diag_log_event(DIAG_SEVERITY_INFO, "renderer-smoke", "started hidden=true size=64x64");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        diag_log_event(DIAG_SEVERITY_ERROR, "renderer-smoke", SDL_GetError());
        result = SMOKE_SDL_INIT;
        goto cleanup;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 0);
    if (borderless) {
        SDL_Rect bounds;
        SDL_DisplayID display = SDL_GetPrimaryDisplay();
        if (!display || !SDL_GetDisplayBounds(display, &bounds)) {
            diag_log_event(DIAG_SEVERITY_ERROR, "renderer-smoke", "desktop_bounds_unavailable");
            result = SMOKE_CONTEXT;
            goto cleanup;
        }
        window_width = bounds.w;
        window_height = bounds.h;
        window_flags |= SDL_WINDOW_BORDERLESS;
        diag_log_event(DIAG_SEVERITY_INFO, "renderer-smoke", "borderless_requested display_mode_change=false");
    }
    window = SDL_CreateWindow("OpenJKDF2 Renderer Smoke", window_width, window_height, window_flags);
    if (!window) {
        diag_log_event(DIAG_SEVERITY_ERROR, "renderer-smoke", SDL_GetError());
        result = SMOKE_CONTEXT;
        goto cleanup;
    }
    context = SDL_GL_CreateContext(window);
    if (!context || !SDL_GL_MakeCurrent(window, context)) {
        diag_log_event(DIAG_SEVERITY_ERROR, "renderer-smoke", SDL_GetError());
        result = SMOKE_CONTEXT;
        goto cleanup;
    }
    if (borderless) {
        int actual_width = 0;
        int actual_height = 0;
        SDL_GetWindowSize(window, &actual_width, &actual_height);
        if (actual_width != window_width || actual_height != window_height ||
            !(SDL_GetWindowFlags(window) & SDL_WINDOW_BORDERLESS)) {
            diag_log_event(DIAG_SEVERITY_ERROR, "renderer-smoke", "borderless_geometry_invalid");
            result = SMOKE_CONTEXT;
            goto cleanup;
        }
    }
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        diag_log_event(DIAG_SEVERITY_ERROR, "renderer-smoke", "glew_initialization_failed");
        result = SMOKE_LOADER;
        goto cleanup;
    }
    glGetError();
    smoke_log_gl_string("vendor", GL_VENDOR);
    smoke_log_gl_string("renderer", GL_RENDERER);
    smoke_log_gl_string("version", GL_VERSION);
    smoke_log_gl_string("glsl", GL_SHADING_LANGUAGE_VERSION);
    if (!smoke_write_report()) {
        diag_log_event(DIAG_SEVERITY_WARNING, "renderer-smoke", "renderer_report_write_failed");
    }

    vertex = shader_compile_named("smoke_vertex", SHADER_STAGE_VERTEX, vertex_source);
    fragment = shader_compile_named("smoke_fragment", SHADER_STAGE_FRAGMENT, fragment_source);
    if (!vertex.ok || !fragment.ok) {
        result = SMOKE_SHADER;
        goto cleanup;
    }
    program = shader_link_named("smoke_program", vertex.object, fragment.object);
    vertex.object = fragment.object = 0;
    if (!program.ok) {
        result = SMOKE_SHADER;
        goto cleanup;
    }

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        diag_log_event(DIAG_SEVERITY_ERROR, "renderer-smoke", "framebuffer_incomplete");
        result = SMOKE_FRAMEBUFFER;
        goto cleanup;
    }
    glViewport(0, 0, 64, 64);
    glUseProgram(program.object);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glReadPixels(32, 32, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    if (glGetError() != GL_NO_ERROR || pixel[0] < 45 || pixel[0] > 57 ||
        pixel[1] < 96 || pixel[1] > 108 || pixel[2] < 147 || pixel[2] > 159 || pixel[3] != 255) {
        char event[128];
        snprintf(event, sizeof(event), "readback_failed rgba=%u,%u,%u,%u", pixel[0], pixel[1], pixel[2], pixel[3]);
        diag_log_event(DIAG_SEVERITY_ERROR, "renderer-smoke", event);
        result = SMOKE_FRAMEBUFFER;
        goto cleanup;
    }
    diag_log_event(DIAG_SEVERITY_INFO, "renderer-smoke", "passed framebuffer=complete draw=triangle readback=valid");

cleanup:
    if (framebuffer) glDeleteFramebuffers(1, &framebuffer);
    if (texture) glDeleteTextures(1, &texture);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
    if (program.object) glDeleteProgram(program.object);
    if (vertex.object) glDeleteShader(vertex.object);
    if (fragment.object) glDeleteShader(fragment.object);
    if (context) SDL_GL_DestroyContext(context);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
    diag_log_finish(result == SMOKE_OK);
    return result;
}
