#ifndef OPENJKDF2_RENDERER_DIAGNOSTICS_H
#define OPENJKDF2_RENDERER_DIAGNOSTICS_H

#include <stdbool.h>
#include <stddef.h>

typedef struct RendererDiagnostics
{
    const char* backend;
    const char* gpu;
    const char* vendor;
    const char* driver;
    const char* api;
    const char* fallback;
    const char* vsync;
    const char* display_mode;
    int width;
    int height;
    int refresh_hz;
    const char* frame_cap;
} RendererDiagnostics;

bool renderer_diagnostics_format(const RendererDiagnostics* diagnostics, char* output, size_t output_size);

#endif
