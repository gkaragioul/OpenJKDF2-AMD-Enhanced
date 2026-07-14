#include "General/RendererDiagnostics.h"

#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(value) do { if (!(value)) { fprintf(stderr, "line %d: %s\n", __LINE__, #value); ++failures; } } while (0)

int main(void)
{
    RendererDiagnostics diagnostics = {
        "OpenGL Core", "AMD Radeon RX 7900 XTX", "ATI Technologies Inc.",
        "AMD 24.6.1", "OpenGL 4.6 / GLSL 4.60", "Standards-compliant",
        "Adaptive (fallback: On)", "Borderless", 2560, 1440, 144, "120 FPS"
    };
    char output[1024];
    char small[16] = "unchanged";

    CHECK(renderer_diagnostics_format(&diagnostics, output, sizeof(output)));
    CHECK(strstr(output, "Renderer: OpenGL Core") != NULL);
    CHECK(strstr(output, "GPU: AMD Radeon RX 7900 XTX") != NULL);
    CHECK(strstr(output, "Vendor: ATI Technologies Inc.") != NULL);
    CHECK(strstr(output, "Driver: AMD 24.6.1") != NULL);
    CHECK(strstr(output, "API: OpenGL 4.6 / GLSL 4.60") != NULL);
    CHECK(strstr(output, "Fallback: Standards-compliant") != NULL);
    CHECK(strstr(output, "VSync: Adaptive (fallback: On)") != NULL);
    CHECK(strstr(output, "Display: Borderless, 2560x1440 @ 144 Hz") != NULL);
    CHECK(strstr(output, "Frame cap: 120 FPS") != NULL);
    CHECK(!renderer_diagnostics_format(&diagnostics, small, sizeof(small)));
    CHECK(small[0] == '\0');
    CHECK(!renderer_diagnostics_format(NULL, output, sizeof(output)));

    diagnostics.gpu = NULL;
    CHECK(renderer_diagnostics_format(&diagnostics, output, sizeof(output)));
    CHECK(strstr(output, "GPU: Not collected") != NULL);

    return failures ? 1 : 0;
}
