#include "General/RendererDiagnostics.h"

#include <stdio.h>

static const char* renderer_diagnostics_value(const char* value)
{
    return value && value[0] ? value : "Not collected";
}

bool renderer_diagnostics_format(const RendererDiagnostics* diagnostics, char* output, size_t output_size)
{
    int written;
    if (!output || output_size == 0)
        return false;
    output[0] = '\0';
    if (!diagnostics)
        return false;

    written = snprintf(output, output_size,
        "Renderer: %s\n"
        "GPU: %s\n"
        "Vendor: %s\n"
        "Driver: %s\n"
        "API: %s\n"
        "Fallback: %s\n"
        "VSync: %s\n"
        "Display: %s, %dx%d @ %d Hz\n"
        "Frame cap: %s",
        renderer_diagnostics_value(diagnostics->backend),
        renderer_diagnostics_value(diagnostics->gpu),
        renderer_diagnostics_value(diagnostics->vendor),
        renderer_diagnostics_value(diagnostics->driver),
        renderer_diagnostics_value(diagnostics->api),
        renderer_diagnostics_value(diagnostics->fallback),
        renderer_diagnostics_value(diagnostics->vsync),
        renderer_diagnostics_value(diagnostics->display_mode),
        diagnostics->width,
        diagnostics->height,
        diagnostics->refresh_hz,
        renderer_diagnostics_value(diagnostics->frame_cap));
    if (written < 0 || (size_t)written >= output_size)
    {
        output[0] = '\0';
        return false;
    }
    return true;
}
