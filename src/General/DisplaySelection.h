#ifndef OPENJKDF2_DISPLAY_SELECTION_H
#define OPENJKDF2_DISPLAY_SELECTION_H

#include "General/DisplayTransaction.h"

#define DISPLAY_SELECTION_MAX_MONITORS 16
#define DISPLAY_SELECTION_MAX_MODES 256
#define DISPLAY_SELECTION_MIN_WIDTH 640
#define DISPLAY_SELECTION_MIN_HEIGHT 480
#define DISPLAY_SELECTION_MAX_DIMENSION 16384

typedef struct DisplayResolution
{
    int width;
    int height;
    int refresh_hz;
} DisplayResolution;

typedef struct DisplayMonitor
{
    int desktop_width;
    int desktop_height;
    int desktop_refresh_hz;
    int mode_count;
    DisplayResolution modes[DISPLAY_SELECTION_MAX_MODES];
} DisplayMonitor;

typedef struct DisplayInventory
{
    int monitor_count;
    int primary_monitor;
    DisplayMonitor monitors[DISPLAY_SELECTION_MAX_MONITORS];
} DisplayInventory;

typedef enum DisplaySelectionReason
{
    DISPLAY_SELECTION_OK = 0,
    DISPLAY_SELECTION_NO_DISPLAYS,
    DISPLAY_SELECTION_MONITOR_FALLBACK,
    DISPLAY_SELECTION_WINDOW_SIZE_CLAMPED,
    DISPLAY_SELECTION_COMPOSITOR_REFRESH,
    DISPLAY_SELECTION_DESKTOP_MODE,
    DISPLAY_SELECTION_EXCLUSIVE_GUARD_UNAVAILABLE,
    DISPLAY_SELECTION_EXCLUSIVE_MODE_UNAVAILABLE,
    DISPLAY_SELECTION_INVALID_MODE
} DisplaySelectionReason;

typedef struct DisplaySelectionResult
{
    int accepted;
    int normalized;
    DisplaySelectionReason reason;
    DisplaySettings settings;
} DisplaySelectionResult;

DisplaySelectionResult display_selection_resolve(
    const DisplayInventory* inventory,
    DisplaySettings requested,
    int restoration_guard_ready);

#endif
