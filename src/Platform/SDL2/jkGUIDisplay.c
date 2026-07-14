#include "Gui/jkGUIDisplay.h"

#include "General/stdBitmap.h"
#include "General/stdFont.h"
#include "General/stdString.h"
#include "Engine/rdMaterial.h" // TODO move tVBuffer
#include "stdPlatform.h"
#include "jk.h"
#include "Gui/jkGUIRend.h"
#include "Gui/jkGUI.h"
#include "Gui/jkGUISetup.h"
#include "Gui/jkGUIDialog.h"
#include "World/jkPlayer.h"
#include "Win95/Window.h"
#include "Platform/std3D.h"
#include "General/FrameRate.h"
#include "General/PresentationMode.h"
#include "General/QualityPreset.h"
#include "General/DisplayTransaction.h"
#include "General/DiagnosticLog.h"
#include "General/VideoDefaults.h"
#include "General/RendererDiagnostics.h"
#include "Main/jkStrings.h"

#include "jk.h"

enum jkGuiDecisionButton_t
{
    GUI_GENERAL = 100,
    GUI_GAMEPLAY = 101,
    GUI_DISPLAY = 102,
    GUI_SOUND = 103,
    GUI_CONTROLS = 104,

    GUI_ADVANCED = 105,
    GUI_ASPECT_OPTIONS = 106,
    GUI_DISPLAY_OPTIONS = 107,
    GUI_SAFE_60 = 4600,
    GUI_RESET_VIDEO = 4601,
    GUI_SAFE_VIDEO = 4602,
    GUI_DIAGNOSTICS = 4603,
};

static char16_t render_level[256] = {0};
static char16_t gamma_level[256] = {0};
static char16_t hud_level[256] = {0};

static char16_t slider_val_text[5] = {0};
static char16_t slider_val_text_2[48] = {0};
static char16_t slider_val_text_3[32] = {0};
static char16_t quality_val_text[32] = {0};
static char16_t anisotropy_val_text[32] = {0};
static char16_t mipmap_bias_text[32] = {0};
static char16_t display_mode_text[64] = {0};
static char16_t display_monitor_text[256] = {0};
static char16_t display_resolution_text[64] = {0};
static char16_t display_width_text[16] = {0};
static char16_t display_height_text[16] = {0};
static char16_t display_refresh_text[64] = {0};
static char16_t display_effective_text[128] = {0};
static DisplayInventory display_inventory;
static char16_t diagnostics_lines[9][256] = {{0}};

static int slider_images[2] = {JKGUI_BM_SLIDER_BACK, JKGUI_BM_SLIDER_THUMB};

void jkGuiDisplay_FovDraw(jkGuiElement *element, jkGuiMenu *menu, tVBuffer *vbuf, int redraw);
void jkGuiDisplay_FramelimitDraw(jkGuiElement *element, jkGuiMenu *menu, tVBuffer *vbuf, int redraw);
void jkGuiDisplay_VsyncDraw(jkGuiElement *element, jkGuiMenu *menu, tVBuffer *vbuf, int redraw);
void jkGuiDisplay_QualityDraw(jkGuiElement *element, jkGuiMenu *menu, tVBuffer *vbuf, int redraw);
void jkGuiDisplay_AnisotropyDraw(jkGuiElement *element, jkGuiMenu *menu, tVBuffer *vbuf, int redraw);
void jkGuiDisplay_DisplayModeDraw(jkGuiElement *element, jkGuiMenu *menu, tVBuffer *vbuf, int redraw);
void jkGuiDisplay_DisplayMonitorDraw(jkGuiElement *element, jkGuiMenu *menu, tVBuffer *vbuf, int redraw);
void jkGuiDisplay_DisplayResolutionDraw(jkGuiElement *element, jkGuiMenu *menu, tVBuffer *vbuf, int redraw);
void jkGuiDisplay_DisplayRefreshDraw(jkGuiElement *element, jkGuiMenu *menu, tVBuffer *vbuf, int redraw);
static int jkGuiDisplay_ApplyDisplayChange(DisplaySettings proposed);

static jkGuiElement jkGuiDisplay_aElements[32] = {
    { ELEMENT_TEXT,        0,            0, NULL,                   3, {0, 410, 640, 20},   1, 0, NULL,                        0, 0, 0, {0}, 0},
    { ELEMENT_TEXT,        0,            6, "GUI_SETUP",            3, {20, 20, 600, 40},   1, 0, NULL,                        0, 0, 0, {0}, 0},
    { ELEMENT_TEXTBUTTON,  GUI_GENERAL,  2, "GUI_GENERAL",          3, {20, 80, 120, 40},   1, 0, "GUI_GENERAL_HINT",          0, 0, 0, {0}, 0},
    { ELEMENT_TEXTBUTTON,  GUI_GAMEPLAY, 2, "GUI_GAMEPLAY",         3, {140, 80, 120, 40},  1, 0, "GUI_GAMEPLAY_HINT",         0, 0, 0, {0}, 0},
    { ELEMENT_TEXTBUTTON,  GUI_DISPLAY,  2, "GUI_DISPLAY",          3, {260, 80, 120, 40},  1, 0, "GUI_DISPLAY_HINT",          0, 0, 0, {0}, 0},
    { ELEMENT_TEXTBUTTON,  GUI_SOUND,    2, "GUI_SOUND",            3, {380, 80, 120, 40},  1, 0, "GUI_SOUND_HINT",            0, 0, 0, {0}, 0},
    { ELEMENT_TEXTBUTTON,  GUI_CONTROLS, 2, "GUI_CONTROLS",         3, {500, 80, 120, 40},  1, 0, "GUI_CONTROLS_HINT",         0, 0, 0, {0}, 0},
    { ELEMENT_TEXTBUTTON,  1,            2, "GUIEXT_APPLY",         3, {440, 430, 200, 40}, 1, 0, NULL,                        0, 0, 0, {0}, 0},
    { ELEMENT_TEXTBUTTON, -1,            2, "GUI_CANCEL",           3, {0, 430, 200, 40},   1, 0, NULL,                        0, 0, 0, {0}, 0},

    // 9
    {ELEMENT_TEXT,         0,            0, "GUIEXT_FOV",                 3, {20, 130, 300, 30}, 1,  0, 0, 0, 0, 0, {0}, 0},
    {ELEMENT_SLIDER,       0,            0, (const char*)(FOV_MAX - FOV_MIN),                    0, {10, 160, 320, 30}, 1, 0, "GUIEXT_FOV_HINT", jkGuiDisplay_FovDraw, 0, slider_images, {0}, 0},
    {ELEMENT_TEXT,         0,            0, slider_val_text,        3, {20, 190, 300, 30}, 1,  0, 0, 0, 0, 0, {0}, 0},
    {ELEMENT_CHECKBOX,     0,            0, "GUIEXT_FOV_VERTICAL",    0, {20, 210, 200, 40}, 1,  0, NULL, 0, 0, 0, {0}, 0},
    {ELEMENT_TEXTBUTTON,  GUI_DISPLAY_OPTIONS, 2, "GUIEXT_DISPLAY_OPTIONS", 3, {380, 145, 240, 40}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    {ELEMENT_CHECKBOX,     0,            0, "GUIEXT_EN_HIDPI",    0, {400, 180, 200, 40}, 1,  0, NULL, 0, 0, 0, {0}, 0},
    {ELEMENT_CHECKBOX,     0,            0, "GUIEXT_EN_TEXTURE_FILTERING",    0, {400, 210, 200, 40}, 1,  0, NULL, 0, 0, 0, {0}, 0},
    {ELEMENT_TEXTBUTTON,  GUI_ASPECT_OPTIONS, 2, "GUIEXT_ASPECT_OPTIONS", 3, {20, 240, 300, 40}, 1, 0, NULL, 0, 0, 0, {0}, 0},

    // 17
    {ELEMENT_TEXT,         0,            0, "GUIEXT_FPS_LIMIT",                 3, {20, 280, 300, 30}, 1,  0, 0, 0, 0, 0, {0}, 0},
    {ELEMENT_SLIDER,       0,            0, (const char*)FRAME_RATE_SLIDER_DESKTOP,                    0, {10, 310, 320, 30}, 1, 0, "GUIEXT_FPS_LIMIT_HINT", jkGuiDisplay_FramelimitDraw, 0, slider_images, {0}, 0},
    {ELEMENT_TEXT,         0,            0, slider_val_text_2,        3, {20, 340, 300, 30}, 1,  0, 0, 0, 0, 0, {0}, 0},
    {ELEMENT_TEXT,         0,            0, slider_val_text_3,        3, {20, 370, 150, 30}, 1,  0, 0, 0, 0, 0, {0}, 0},
    {ELEMENT_SLIDER,       0,            0, (const char*)2,                    0, {160, 370, 170, 30}, 1, 0, "GUIEXT_EN_VSYNC", jkGuiDisplay_VsyncDraw, 0, slider_images, {0}, 0},
    
    // 22
    {ELEMENT_CHECKBOX,     0,            0, "GUIEXT_EN_BLOOM",    0, {400, 240, 300, 40}, 1,  0, NULL, 0, 0, 0, {0}, 0},

    // 22
    {ELEMENT_CHECKBOX,     0,            0, "GUIEXT_EN_SSAO",    0, {400, 270, 300, 40}, 1,  0, NULL, 0, 0, 0, {0}, 0},
    
    // 23
    { ELEMENT_TEXT,        0,            0, "GUIEXT_SSAA_MULT",            2, {400, 320, 120, 20},   1, 0, NULL,                        0, 0, 0, {0}, 0},
    { ELEMENT_TEXTBOX,      0,            0, NULL,    100, {530, 320, 80, 20}, 1,  0, NULL, 0, 0, 0, {0}, 0},
    
    // 25
    { ELEMENT_TEXT,        0,            0, "GUIEXT_GAMMA_VAL",            2, {400, 350, 120, 20},   1, 0, NULL,                        0, 0, 0, {0}, 0},
    { ELEMENT_TEXTBOX,      0,            0, NULL,    100, {530, 350, 80, 20}, 1,  0, NULL, 0, 0, 0, {0}, 0},

    // 27
    { ELEMENT_TEXT,        0,            0, "GUIEXT_HUD_SCALE",            2, {400, 380, 120, 20},   1, 0, NULL,                        0, 0, 0, {0}, 0},
    { ELEMENT_TEXTBOX,      0,            0, NULL,    100, {530, 380, 80, 20}, 1,  0, NULL, 0, 0, 0, {0}, 0},

    { ELEMENT_TEXTBUTTON,  GUI_ADVANCED, 2, "GUI_ADVANCED",               3, {220, 430, 200, 40}, 1, 0, NULL,                        0, 0, 0, {0}, 0},

    { ELEMENT_END,         0,            0, NULL,                   0, {0},                 0, 0, NULL,                        0, 0, 0, {0}, 0},
};

static jkGuiMenu jkGuiDisplay_menu = { jkGuiDisplay_aElements, 0, 0xFF, 0xE1, 0x0F, 0, 0, jkGui_stdBitmaps, jkGui_stdFonts, 0, 0, "thermloop01.wav", "thrmlpu2.wav", 0, 0, 0, 0, 0, 0 };

static jkGuiElement jkGuiDisplay_aElementsAdvanced[25] = {
    { ELEMENT_TEXT,        0,            0, NULL,                   3, {0, 410, 640, 20},   1, 0, NULL,                        0, 0, 0, {0}, 0},
    { ELEMENT_TEXT,        0,            6, "GUI_SETUP",            3, {20, 20, 600, 40},   1, 0, NULL,                        0, 0, 0, {0}, 0},
    { ELEMENT_TEXTBUTTON,  GUI_GENERAL,  2, "GUI_GENERAL",          3, {20, 80, 120, 40},   1, 0, "GUI_GENERAL_HINT",          0, 0, 0, {0}, 0},
    { ELEMENT_TEXTBUTTON,  GUI_GAMEPLAY, 2, "GUI_GAMEPLAY",         3, {140, 80, 120, 40},  1, 0, "GUI_GAMEPLAY_HINT",         0, 0, 0, {0}, 0},
    { ELEMENT_TEXTBUTTON,  GUI_DISPLAY,  2, "GUI_DISPLAY",          3, {260, 80, 120, 40},  1, 0, "GUI_DISPLAY_HINT",          0, 0, 0, {0}, 0},
    { ELEMENT_TEXTBUTTON,  GUI_SOUND,    2, "GUI_SOUND",            3, {380, 80, 120, 40},  1, 0, "GUI_SOUND_HINT",            0, 0, 0, {0}, 0},
    { ELEMENT_TEXTBUTTON,  GUI_CONTROLS, 2, "GUI_CONTROLS",         3, {500, 80, 120, 40},  1, 0, "GUI_CONTROLS_HINT",         0, 0, 0, {0}, 0},
    
    { ELEMENT_TEXTBUTTON,  1,            2, "GUIEXT_APPLY",         3, {440, 430, 200, 40}, 1, 0, NULL,                        0, 0, 0, {0}, 0},
    { ELEMENT_TEXTBUTTON, -1,            2, "GUI_CANCEL",           3, {0, 430, 200, 40},   1, 0, NULL,                        0, 0, 0, {0}, 0},
    
    { ELEMENT_CHECKBOX,    0,            0, "GUIEXT_EN_JKGFXMOD",            0, {20, 150, 300, 40},  1, 0, "GUIEXT_EN_JKGFXMOD_HINT",          0, 0, 0, {0}, 0},
    { ELEMENT_CHECKBOX,    0,            0, "GUIEXT_EN_TEXTURE_PRECACHE",   0, {20, 190, 300, 40},  1, 0, "GUIEXT_EN_TEXTURE_PRECACHE_HINT",          0, 0, 0, {0}, 0},
    { ELEMENT_CHECKBOX,    0,            0, "GUIEXT_SHOW_FRAME_STATS",      0, {20, 230, 300, 40},  1, 0, "GUIEXT_SHOW_FRAME_STATS_HINT",             0, 0, 0, {0}, 0},
    { ELEMENT_TEXTBUTTON,  GUI_SAFE_60,  2, "GUIEXT_SAFE_60",               3, {20, 290, 300, 40},  1, 0, "GUIEXT_SAFE_60_HINT",                      0, 0, 0, {0}, 0},
    { ELEMENT_TEXTBUTTON,  GUI_RESET_VIDEO, 2, "GUIEXT_RESET_VIDEO",         3, {20, 335, 300, 35},  1, 0, "GUIEXT_RESET_VIDEO_HINT",                  0, 0, 0, {0}, 0},
    { ELEMENT_TEXTBUTTON,  GUI_SAFE_VIDEO, 2, "GUIEXT_SAFE_VIDEO",           3, {20, 375, 300, 35},  1, 0, "GUIEXT_SAFE_VIDEO_HINT",                   0, 0, 0, {0}, 0},
    { ELEMENT_TEXT,        0,            0, "GUIEXT_QUALITY_PRESET",         2, {350, 145, 130, 20}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_SLIDER,      0,            0, (const char*)QUALITY_PRESET_CUSTOM, 0, {350, 170, 250, 24}, 1, 0, "GUIEXT_QUALITY_PRESET_HINT", jkGuiDisplay_QualityDraw, 0, slider_images, {0}, 0},
    { ELEMENT_TEXT,        0,            0, quality_val_text,                 3, {350, 195, 250, 20}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_TEXT,        0,            0, "GUIEXT_ANISOTROPY",             2, {350, 225, 130, 20}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_SLIDER,      0,            0, (const char*)4,                   0, {350, 250, 250, 24}, 1, 0, "GUIEXT_ANISOTROPY_HINT", jkGuiDisplay_AnisotropyDraw, 0, slider_images, {0}, 0},
    { ELEMENT_TEXT,        0,            0, anisotropy_val_text,              3, {350, 275, 250, 20}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_TEXT,        0,            0, "GUIEXT_MIPMAP_BIAS",            2, {350, 310, 130, 20}, 1, 0, "GUIEXT_MIPMAP_BIAS_HINT", 0, 0, 0, {0}, 0},
    { ELEMENT_TEXTBOX,     0,            0, mipmap_bias_text,                16, {490, 307, 100, 24}, 1, 0, "GUIEXT_MIPMAP_BIAS_HINT", 0, 0, 0, {0}, 0},
    { ELEMENT_TEXTBUTTON,  GUI_DIAGNOSTICS, 2, "GUIEXT_DIAGNOSTICS",          3, {350, 355, 250, 40}, 1, 0, "GUIEXT_DIAGNOSTICS_HINT", 0, 0, 0, {0}, 0},
    
    { ELEMENT_END,         0,            0, NULL,                   0, {0},                 0, 0, NULL,                        0, 0, 0, {0}, 0},
};

static jkGuiMenu jkGuiDisplay_menuAdvanced = { jkGuiDisplay_aElementsAdvanced, 0, 0xFF, 0xE1, 0x0F, 0, 0, jkGui_stdBitmaps, jkGui_stdFonts, 0, 0, "thermloop01.wav", "thrmlpu2.wav", 0, 0, 0, 0, 0, 0 };

static jkGuiElement jkGuiDisplay_diagnosticsElements[12] = {
    { ELEMENT_TEXT,       0, 6, "GUIEXT_DIAGNOSTICS_TITLE", 3, {20, 20, 600, 40}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_TEXT,       0, 0, diagnostics_lines[0], 2, {40, 90, 560, 28}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_TEXT,       0, 0, diagnostics_lines[1], 2, {40, 122, 560, 28}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_TEXT,       0, 0, diagnostics_lines[2], 2, {40, 154, 560, 28}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_TEXT,       0, 0, diagnostics_lines[3], 2, {40, 186, 560, 28}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_TEXT,       0, 0, diagnostics_lines[4], 2, {40, 218, 560, 28}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_TEXT,       0, 0, diagnostics_lines[5], 2, {40, 250, 560, 28}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_TEXT,       0, 0, diagnostics_lines[6], 2, {40, 282, 560, 28}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_TEXT,       0, 0, diagnostics_lines[7], 2, {40, 314, 560, 28}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_TEXT,       0, 0, diagnostics_lines[8], 2, {40, 346, 560, 28}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_TEXTBUTTON, 1, 2, "GUI_OK", 3, {440, 430, 200, 40}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_END,        0, 0, NULL, 0, {0}, 0, 0, NULL, 0, 0, 0, {0}, 0},
};
static jkGuiMenu jkGuiDisplay_diagnosticsMenu = { jkGuiDisplay_diagnosticsElements, 0, 0xFF, 0xE1, 0x0F, 0, 0, jkGui_stdBitmaps, jkGui_stdFonts, 0, 0, "thermloop01.wav", "thrmlpu2.wav", 0, 0, 0, 0, 0, 0 };

static jkGuiElement jkGuiDisplay_aspectElements[8] = {
    { ELEMENT_TEXT,       0, 6, "GUIEXT_ASPECT_OPTIONS", 3, {20, 20, 600, 40}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_CHECKBOX,   0, 0, "GUIEXT_ASPECT_GAMEPLAY", 0, {80, 110, 480, 45}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_CHECKBOX,   0, 0, "GUIEXT_ASPECT_MENUS", 0, {80, 165, 480, 45}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_CHECKBOX,   0, 0, "GUIEXT_ASPECT_HUD", 0, {80, 220, 480, 45}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_CHECKBOX,   0, 0, "GUIEXT_ASPECT_VIDEOS", 0, {80, 275, 480, 45}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_TEXTBUTTON, 1, 2, "GUI_OK", 3, {440, 430, 200, 40}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_TEXTBUTTON, -1, 2, "GUI_CANCEL", 3, {0, 430, 200, 40}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_END,        0, 0, NULL, 0, {0}, 0, 0, NULL, 0, 0, 0, {0}, 0},
};
static jkGuiMenu jkGuiDisplay_aspectMenu = { jkGuiDisplay_aspectElements, 0, 0xFF, 0xE1, 0x0F, 0, 0, jkGui_stdBitmaps, jkGui_stdFonts, 0, 0, "thermloop01.wav", "thrmlpu2.wav", 0, 0, 0, 0, 0, 0 };

static jkGuiElement jkGuiDisplay_displayOptionsElements[23] = {
    { ELEMENT_TEXT,       0, 6, "GUIEXT_DISPLAY_OPTIONS", 3, {20, 20, 600, 40}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_TEXT,       0, 0, "GUIEXT_DISPLAY_MODE", 2, {40, 80, 150, 24}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_SLIDER,     0, 0, (const char*)2, 0, {190, 80, 260, 24}, 1, 0, NULL, jkGuiDisplay_DisplayModeDraw, 0, slider_images, {0}, 0},
    { ELEMENT_TEXT,       0, 0, display_mode_text, 3, {460, 80, 160, 24}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_TEXT,       0, 0, "GUIEXT_DISPLAY_MONITOR", 2, {40, 120, 150, 24}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_SLIDER,     0, 0, (const char*)0, 0, {190, 120, 260, 24}, 1, 0, NULL, jkGuiDisplay_DisplayMonitorDraw, 0, slider_images, {0}, 0},
    { ELEMENT_TEXT,       0, 0, display_monitor_text, 3, {460, 120, 160, 24}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_TEXT,       0, 0, "GUIEXT_DISPLAY_RESOLUTION", 2, {40, 160, 150, 24}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_SLIDER,     0, 0, (const char*)0, 0, {190, 160, 260, 24}, 1, 0, NULL, jkGuiDisplay_DisplayResolutionDraw, 0, slider_images, {0}, 0},
    { ELEMENT_TEXT,       0, 0, display_resolution_text, 3, {460, 160, 160, 24}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_TEXT,       0, 0, "GUIEXT_DISPLAY_WIDTH", 2, {40, 205, 100, 24}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_TEXTBOX,    0, 0, display_width_text, 8, {140, 202, 100, 24}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_TEXT,       0, 0, "GUIEXT_DISPLAY_HEIGHT", 2, {270, 205, 100, 24}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_TEXTBOX,    0, 0, display_height_text, 8, {370, 202, 100, 24}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_TEXT,       0, 0, "GUIEXT_DISPLAY_REFRESH", 2, {40, 250, 150, 24}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_SLIDER,     0, 0, (const char*)1, 0, {190, 250, 260, 24}, 1, 0, NULL, jkGuiDisplay_DisplayRefreshDraw, 0, slider_images, {0}, 0},
    { ELEMENT_TEXT,       0, 0, display_refresh_text, 3, {460, 250, 160, 24}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_TEXT,       0, 0, "GUIEXT_DISPLAY_EFFECTIVE", 2, {40, 300, 150, 24}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_TEXT,       0, 0, display_effective_text, 3, {190, 300, 420, 24}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_TEXTBUTTON, 1, 2, "GUIEXT_APPLY", 3, {440, 430, 200, 40}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_TEXTBUTTON,-1, 2, "GUI_CANCEL", 3, {0, 430, 200, 40}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_TEXT,       0, 0, "GUIEXT_EXCLUSIVE_UNAVAILABLE", 2, {40, 350, 560, 35}, 1, 0, NULL, 0, 0, 0, {0}, 0},
    { ELEMENT_END,        0, 0, NULL, 0, {0}, 0, 0, NULL, 0, 0, 0, {0}, 0},
};
static jkGuiMenu jkGuiDisplay_displayOptionsMenu = { jkGuiDisplay_displayOptionsElements, 0, 0xFF, 0xE1, 0x0F, 0, 0, jkGui_stdBitmaps, jkGui_stdFonts, 0, 0, "thermloop01.wav", "thrmlpu2.wav", 0, 0, 0, 0, 0, 0 };


static DisplayMonitor* jkGuiDisplay_SelectedMonitor(void)
{
    int monitor = jkGuiDisplay_displayOptionsElements[5].selectedTextEntry;
    if (monitor < 0 || monitor >= display_inventory.monitor_count) monitor = 0;
    return &display_inventory.monitors[monitor];
}

void jkGuiDisplay_DisplayModeDraw(jkGuiElement *element, jkGuiMenu *menu, tVBuffer *vbuf, int redraw)
{
    static const char16_t* names[] = {u"Windowed", u"Borderless", u"Exclusive"};
    int mode = element->selectedTextEntry;
    if (mode < DISPLAY_MODE_WINDOWED || mode > DISPLAY_MODE_EXCLUSIVE) mode = DISPLAY_MODE_WINDOWED;
    if (mode == DISPLAY_MODE_EXCLUSIVE && !Window_IsRestorationGuardReady())
        jk_snwprintf(display_mode_text, 64, u"Exclusive (Unavailable)");
    else
        jk_snwprintf(display_mode_text, 64, u"%ls", names[mode]);
    jkGuiDisplay_displayOptionsElements[21].bIsVisible = !Window_IsRestorationGuardReady();
    jkGuiRend_SliderDraw(element, menu, vbuf, redraw);
    jkGuiRend_UpdateAndDrawClickable(&jkGuiDisplay_displayOptionsElements[3], menu, 1);
}

void jkGuiDisplay_DisplayMonitorDraw(jkGuiElement *element, jkGuiMenu *menu, tVBuffer *vbuf, int redraw)
{
    int monitor = element->selectedTextEntry;
    char16_t name[192];
    if (monitor < 0 || monitor >= display_inventory.monitor_count) monitor = 0;
    element->selectedTextEntry = monitor;
    stdString_CharToWchar(name, Window_GetDisplayName(monitor), 191);
    jk_snwprintf(display_monitor_text, 256, u"%d - %ls", monitor + 1, name);
    jkGuiRend_SliderDraw(element, menu, vbuf, redraw);
    jkGuiRend_UpdateAndDrawClickable(&jkGuiDisplay_displayOptionsElements[6], menu, 1);
}

void jkGuiDisplay_DisplayResolutionDraw(jkGuiElement *element, jkGuiMenu *menu, tVBuffer *vbuf, int redraw)
{
    DisplayMonitor* monitor = jkGuiDisplay_SelectedMonitor();
    int mode = jkGuiDisplay_displayOptionsElements[2].selectedTextEntry;
    int selection = element->selectedTextEntry;
    element->extraInt = monitor->mode_count > 0 ? monitor->mode_count : 1;
    if (selection < 0 || selection > monitor->mode_count) selection = monitor->mode_count;
    element->selectedTextEntry = selection;
    if (mode == DISPLAY_MODE_BORDERLESS)
    {
        jk_snwprintf(display_width_text, 16, u"%d", monitor->desktop_width);
        jk_snwprintf(display_height_text, 16, u"%d", monitor->desktop_height);
        jk_snwprintf(display_resolution_text, 64, u"Desktop %dx%d", monitor->desktop_width, monitor->desktop_height);
        element->enableHover = 0;
    }
    else if (selection < monitor->mode_count)
    {
        DisplayResolution selected = monitor->modes[selection];
        jk_snwprintf(display_width_text, 16, u"%d", selected.width);
        jk_snwprintf(display_height_text, 16, u"%d", selected.height);
        jk_snwprintf(display_resolution_text, 64, u"%dx%d", selected.width, selected.height);
        element->enableHover = 1;
    }
    else
    {
        jk_snwprintf(display_resolution_text, 64, u"Custom");
        element->enableHover = mode == DISPLAY_MODE_WINDOWED;
    }
    jkGuiDisplay_displayOptionsElements[11].enableHover =
        jkGuiDisplay_displayOptionsElements[13].enableHover =
            mode == DISPLAY_MODE_WINDOWED && selection >= monitor->mode_count;
    jk_snwprintf(display_effective_text, 128, u"Monitor %d - %ls - %ls",
                 jkGuiDisplay_displayOptionsElements[5].selectedTextEntry + 1,
                 display_resolution_text, mode == DISPLAY_MODE_WINDOWED
                    ? u"Compositor" : (mode == DISPLAY_MODE_EXCLUSIVE
                    ? u"Requested" : u"Desktop"));
    jkGuiRend_SliderDraw(element, menu, vbuf, redraw);
    jkGuiRend_UpdateAndDrawClickable(&jkGuiDisplay_displayOptionsElements[9], menu, 1);
    jkGuiRend_UpdateAndDrawClickable(&jkGuiDisplay_displayOptionsElements[11], menu, 1);
    jkGuiRend_UpdateAndDrawClickable(&jkGuiDisplay_displayOptionsElements[13], menu, 1);
    jkGuiRend_UpdateAndDrawClickable(&jkGuiDisplay_displayOptionsElements[18], menu, 1);
}

void jkGuiDisplay_DisplayRefreshDraw(jkGuiElement *element, jkGuiMenu *menu, tVBuffer *vbuf, int redraw)
{
    int mode = jkGuiDisplay_displayOptionsElements[2].selectedTextEntry;
    DisplayMonitor* monitor = jkGuiDisplay_SelectedMonitor();
    int resolution = jkGuiDisplay_displayOptionsElements[8].selectedTextEntry;
    if (mode == DISPLAY_MODE_EXCLUSIVE)
    {
        int refresh = resolution < monitor->mode_count ? monitor->modes[resolution].refresh_hz : 0;
        element->selectedTextEntry = 1;
        element->enableHover = 0;
        jk_snwprintf(display_refresh_text, 64, u"Requested %d Hz", refresh);
    }
    else
    {
        element->selectedTextEntry = 0;
        element->enableHover = 0;
        jk_snwprintf(display_refresh_text, 64, mode == DISPLAY_MODE_BORDERLESS
            ? u"Desktop %d Hz" : u"Compositor", monitor->desktop_refresh_hz);
    }
    jkGuiRend_SliderDraw(element, menu, vbuf, redraw);
    jkGuiRend_UpdateAndDrawClickable(&jkGuiDisplay_displayOptionsElements[16], menu, 1);
}

static int jkGuiDisplay_ShowDisplayOptions(void)
{
    DisplaySettings current = Window_GetDisplaySettings();
    DisplayMonitor* monitor;
    int index;
    int clicked;
    if (!Window_GetDisplayInventory(&display_inventory)) return 0;
    if (current.monitor < 0 || current.monitor >= display_inventory.monitor_count)
        current.monitor = display_inventory.primary_monitor;
    monitor = &display_inventory.monitors[current.monitor];
    jkGuiDisplay_displayOptionsElements[2].selectedTextEntry = current.mode;
    jkGuiDisplay_displayOptionsElements[5].selectedTextEntry = current.monitor;
    jkGuiDisplay_displayOptionsElements[5].extraInt =
        display_inventory.monitor_count > 1 ? display_inventory.monitor_count - 1 : 1;
    jkGuiDisplay_displayOptionsElements[8].selectedTextEntry = monitor->mode_count;
    for (index = 0; index < monitor->mode_count; ++index)
    {
        DisplayResolution mode = monitor->modes[index];
        if (mode.width == current.width && mode.height == current.height &&
            (current.mode != DISPLAY_MODE_EXCLUSIVE || mode.refresh_hz == current.refresh_hz))
        {
            jkGuiDisplay_displayOptionsElements[8].selectedTextEntry = index;
            break;
        }
    }
    jk_snwprintf(display_width_text, 16, u"%d", current.width);
    jk_snwprintf(display_height_text, 16, u"%d", current.height);
    jkGuiDisplay_displayOptionsElements[21].bIsVisible = !Window_IsRestorationGuardReady();
    jkGuiRend_MenuSetReturnKeyShortcutElement(&jkGuiDisplay_displayOptionsMenu, &jkGuiDisplay_displayOptionsElements[19]);
    jkGuiRend_MenuSetEscapeKeyShortcutElement(&jkGuiDisplay_displayOptionsMenu, &jkGuiDisplay_displayOptionsElements[20]);
    jkGuiSetup_sub_412EF0(&jkGuiDisplay_displayOptionsMenu, 0);
    clicked = jkGuiRend_DisplayAndReturnClicked(&jkGuiDisplay_displayOptionsMenu);
    if (clicked == 1)
    {
        DisplaySettings proposed = current;
        char value[32];
        int resolution = jkGuiDisplay_displayOptionsElements[8].selectedTextEntry;
        proposed.mode = (DisplayMode)jkGuiDisplay_displayOptionsElements[2].selectedTextEntry;
        proposed.monitor = jkGuiDisplay_displayOptionsElements[5].selectedTextEntry;
        monitor = &display_inventory.monitors[proposed.monitor];
        if (proposed.mode == DISPLAY_MODE_BORDERLESS)
        {
            proposed.width = monitor->desktop_width;
            proposed.height = monitor->desktop_height;
            proposed.refresh_hz = monitor->desktop_refresh_hz;
        }
        else if (resolution >= 0 && resolution < monitor->mode_count)
        {
            proposed.width = monitor->modes[resolution].width;
            proposed.height = monitor->modes[resolution].height;
            proposed.refresh_hz = proposed.mode == DISPLAY_MODE_EXCLUSIVE ? monitor->modes[resolution].refresh_hz : 0;
        }
        else
        {
            stdString_WcharToChar(value, display_width_text, sizeof(value));
            if (_sscanf(value, "%d", &proposed.width) != 1) proposed.width = current.width;
            stdString_WcharToChar(value, display_height_text, sizeof(value));
            if (_sscanf(value, "%d", &proposed.height) != 1) proposed.height = current.height;
            proposed.refresh_hz = 0;
        }
        return jkGuiDisplay_ApplyDisplayChange(proposed);
    }
    return 0;
}

static int jkGuiDisplay_ShowAspectOptions(void)
{
    int clicked;

    jkGuiDisplay_aspectElements[1].selectedTextEntry = jkPlayer_enableOrigAspect;
    jkGuiDisplay_aspectElements[2].selectedTextEntry = jkPlayer_preserveMenuAspect;
    jkGuiDisplay_aspectElements[3].selectedTextEntry = jkPlayer_preserveHudAspect;
    jkGuiDisplay_aspectElements[4].selectedTextEntry = jkPlayer_preserveVideoAspect;
    jkGuiRend_MenuSetReturnKeyShortcutElement(&jkGuiDisplay_aspectMenu, &jkGuiDisplay_aspectElements[5]);
    jkGuiRend_MenuSetEscapeKeyShortcutElement(&jkGuiDisplay_aspectMenu, &jkGuiDisplay_aspectElements[6]);
    jkGuiSetup_sub_412EF0(&jkGuiDisplay_aspectMenu, 0);
    clicked = jkGuiRend_DisplayAndReturnClicked(&jkGuiDisplay_aspectMenu);
    if (clicked == 1)
    {
        jkPlayer_enableOrigAspect = jkGuiDisplay_aspectElements[1].selectedTextEntry;
        jkPlayer_preserveMenuAspect = jkGuiDisplay_aspectElements[2].selectedTextEntry;
        jkPlayer_preserveHudAspect = jkGuiDisplay_aspectElements[3].selectedTextEntry;
        jkPlayer_preserveVideoAspect = jkGuiDisplay_aspectElements[4].selectedTextEntry;
        jkPlayer_WriteConf(jkPlayer_playerShortName);
    }
    return clicked;
}

static void jkGuiDisplay_ShowDiagnostics(void)
{
    RendererDiagnostics diagnostics;
    char formatted[2048];
    char* line;
    int lineIndex = 0;
    Window_GetRendererDiagnostics(&diagnostics);
    if (!renderer_diagnostics_format(&diagnostics, formatted, sizeof(formatted)))
        snprintf(formatted, sizeof(formatted), "Diagnostics unavailable");
    memset(diagnostics_lines, 0, sizeof(diagnostics_lines));
    line = strtok(formatted, "\n");
    while (line && lineIndex < 9)
    {
        stdString_CharToWchar(diagnostics_lines[lineIndex], line, 255);
        line = strtok(NULL, "\n");
        ++lineIndex;
    }
    jkGuiRend_MenuSetReturnKeyShortcutElement(&jkGuiDisplay_diagnosticsMenu, &jkGuiDisplay_diagnosticsElements[10]);
    jkGuiRend_MenuSetEscapeKeyShortcutElement(&jkGuiDisplay_diagnosticsMenu, &jkGuiDisplay_diagnosticsElements[10]);
    jkGuiSetup_sub_412EF0(&jkGuiDisplay_diagnosticsMenu, 0);
    jkGuiRend_DisplayAndReturnClicked(&jkGuiDisplay_diagnosticsMenu);
}

static int jkGuiDisplay_ApplyDisplayChange(DisplaySettings proposed)
{
    DisplaySettings original = Window_GetDisplaySettings();
    DisplayTransaction transaction;
    DisplaySelectionReason reason = DISPLAY_SELECTION_OK;
    display_transaction_init(&transaction, original);

    if (!display_transaction_requires_confirmation(original, proposed))
        return 1;

    if (!Window_ApplyDisplaySettings(proposed, &reason))
    {
        char16_t title[] = u"Display settings";
        char16_t message[128];
        jk_snwprintf(message, 128, u"Display settings could not be applied (reason %d).", (int)reason);
        jkGuiDialog_ErrorDialog(title, message);
        diag_log_event(DIAG_SEVERITY_WARNING, "display", "display_settings_apply_failed");
        return 0;
    }

    display_transaction_begin(&transaction, proposed, SDL_GetTicks(), 15000);
    Window_RecreateSDL2Window();
    if (Window_ConfirmDisplaySettings(15000))
    {
        display_transaction_confirm(&transaction);
        Window_CommitDisplaySettings(Window_GetDisplaySettings());
        diag_log_event(DIAG_SEVERITY_INFO, "display", "display_settings_confirmed");
        return 1;
    }

    display_transaction_cancel(&transaction);
    original = display_transaction_result(&transaction);
    if (!Window_ApplyDisplaySettings(original, &reason))
        diag_log_event(DIAG_SEVERITY_ERROR, "display", "display_settings_restore_failed");
    Window_RecreateSDL2Window();
    diag_log_event(DIAG_SEVERITY_WARNING, "display", "display_settings_reverted");
    return 0;
}

static int jkGuiDisplay_ApplyDefaults(VideoDefaults defaults)
{
    DisplaySettings proposed = Window_GetDisplaySettings();
    proposed.mode = defaults.display_mode;
    proposed.hidpi = defaults.hidpi;
    if (!jkGuiDisplay_ApplyDisplayChange(proposed))
        return 0;

    jkPlayer_fov = defaults.fov;
    jkPlayer_fovIsVertical = defaults.fov_vertical;
    jkPlayer_enableOrigAspect = defaults.original_aspect;
    jkPlayer_preserveMenuAspect = defaults.preserve_menu_aspect;
    jkPlayer_preserveHudAspect = defaults.preserve_hud_aspect;
    jkPlayer_preserveVideoAspect = defaults.preserve_video_aspect;
    jkPlayer_fpslimit = defaults.fps_limit;
    jkPlayer_enableVsync = defaults.vsync;
    jkPlayer_qualityPreset = defaults.quality_preset;
    jkPlayer_enableTextureFilter = defaults.texture_filtering;
    jkPlayer_anisotropy = defaults.anisotropy;
    jkPlayer_mipmapBias = defaults.mipmap_bias;
    jkPlayer_enableBloom = defaults.bloom;
    jkPlayer_enableSSAO = defaults.ssao;
    jkPlayer_ssaaMultiple = defaults.ssaa_multiple;
    jkPlayer_gamma = defaults.gamma;
    jkPlayer_hudScale = defaults.hud_scale;
    jkPlayer_bEnableTexturePrecache = defaults.texture_precache;
    jkPlayer_bEnableJkgm = defaults.asset_enhancements;

    std3D_PurgeEntireTextureCache();
    std3D_UpdateSettings();
    jkPlayer_WriteConf(jkPlayer_playerShortName);
    diag_log_event(DIAG_SEVERITY_INFO, "settings",
                   defaults.display_mode == DISPLAY_MODE_WINDOWED
                       ? "safe_video_defaults_applied" : "recommended_video_defaults_applied");
    return 1;
}


void jkGuiDisplay_Startup()
{
    flex32_t ftmp;
    jkGui_InitMenu(&jkGuiDisplay_menu, jkGui_stdBitmaps[JKGUI_BM_BK_SETUP]);
    jkGui_InitMenu(&jkGuiDisplay_menuAdvanced, jkGui_stdBitmaps[JKGUI_BM_BK_SETUP]);
    jkGui_InitMenu(&jkGuiDisplay_diagnosticsMenu, jkGui_stdBitmaps[JKGUI_BM_BK_SETUP]);
    jkGui_InitMenu(&jkGuiDisplay_aspectMenu, jkGui_stdBitmaps[JKGUI_BM_BK_SETUP]);
    jkGui_InitMenu(&jkGuiDisplay_displayOptionsMenu, jkGui_stdBitmaps[JKGUI_BM_BK_SETUP]);
    jkGuiDisplay_aElements[25].wstr = render_level;

    jkGuiDisplay_aElements[27].wstr = gamma_level;

    jkGuiDisplay_aElements[29].wstr = hud_level;

    ftmp = jkPlayer_ssaaMultiple;
    jk_snwprintf(render_level, 255, u"%.2f", ftmp);
    ftmp = jkPlayer_gamma;
    jk_snwprintf(gamma_level, 255, u"%.2f", ftmp);
    ftmp = jkPlayer_hudScale;
    jk_snwprintf(hud_level, 255, u"%.2f", ftmp);
}

void jkGuiDisplay_Shutdown()
{
    ;
}

void jkGuiDisplay_FovDraw(jkGuiElement *element, jkGuiMenu *menu, tVBuffer *vbuf, int redraw)
{
    uint32_t tmp = FOV_MIN + jkGuiDisplay_aElements[10].selectedTextEntry;
    
    jk_snwprintf(slider_val_text, 5, u"%u", tmp);
    jkGuiDisplay_aElements[11].wstr = slider_val_text;
    
    jkGuiRend_SliderDraw(element, menu, vbuf, redraw);
    
    jkGuiRend_UpdateAndDrawClickable(&jkGuiDisplay_aElements[11], menu, 1);
}

void jkGuiDisplay_FramelimitDraw(jkGuiElement *element, jkGuiMenu *menu, tVBuffer *vbuf, int redraw)
{
    int tmp = FrameRate_ValueFromSlider(jkGuiDisplay_aElements[18].selectedTextEntry);
    
    if (tmp == FRAME_RATE_DESKTOP_REFRESH)
        jk_snwprintf(slider_val_text_2, 48, u"Desktop Refresh (timing caution)");
    else if (tmp == FRAME_RATE_UNLIMITED)
        jk_snwprintf(slider_val_text_2, 48, u"Unlimited (timing caution)");
    else if (FrameRate_HasTimingCaution(tmp))
        jk_snwprintf(slider_val_text_2, 48, u"%d FPS (timing caution)", tmp);
    else
        jk_snwprintf(slider_val_text_2, 48, u"%d FPS", tmp);

    jkGuiDisplay_aElements[19].wstr = slider_val_text_2;
    
    jkGuiRend_SliderDraw(element, menu, vbuf, redraw);
    
    jkGuiRend_UpdateAndDrawClickable(&jkGuiDisplay_aElements[19], menu, 1);
}

void jkGuiDisplay_VsyncDraw(jkGuiElement *element, jkGuiMenu *menu, tVBuffer *vbuf, int redraw)
{
    PresentationVsyncMode mode = PresentationMode_VsyncFromSlider(jkGuiDisplay_aElements[21].selectedTextEntry);

    if (mode == PRESENTATION_VSYNC_ADAPTIVE)
        jk_snwprintf(slider_val_text_3, 32, u"VSync: Adaptive");
    else if (mode == PRESENTATION_VSYNC_ON)
        jk_snwprintf(slider_val_text_3, 32, u"VSync: On");
    else
        jk_snwprintf(slider_val_text_3, 32, u"VSync: Off");

    jkGuiDisplay_aElements[20].wstr = slider_val_text_3;
    jkGuiRend_SliderDraw(element, menu, vbuf, redraw);
    jkGuiRend_UpdateAndDrawClickable(&jkGuiDisplay_aElements[20], menu, 1);
}

void jkGuiDisplay_QualityDraw(jkGuiElement *element, jkGuiMenu *menu, tVBuffer *vbuf, int redraw)
{
    static const char16_t* names[] = {u"Classic", u"Balanced", u"High", u"Ultra", u"Custom"};
    int preset = jkGuiDisplay_aElementsAdvanced[16].selectedTextEntry;
    if (preset < QUALITY_PRESET_CLASSIC || preset > QUALITY_PRESET_CUSTOM)
        preset = QUALITY_PRESET_CUSTOM;
    jk_snwprintf(quality_val_text, 32, u"%ls", names[preset]);
    if (preset != QUALITY_PRESET_CUSTOM)
    {
        QualityPresetSettings settings = QualityPreset_Get(preset);
        jkGuiDisplay_aElementsAdvanced[9].selectedTextEntry = settings.assetEnhancements;
        jkGuiDisplay_aElementsAdvanced[10].selectedTextEntry = settings.texturePrecache;
        jkGuiDisplay_aElementsAdvanced[19].selectedTextEntry = QualityPreset_SliderFromAnisotropy(settings.anisotropy);
        jk_snwprintf(mipmap_bias_text, 32, u"%.2f", settings.mipmapBias);
        jkGuiDisplay_aElements[15].selectedTextEntry = settings.textureFiltering;
        jkGuiDisplay_aElements[22].selectedTextEntry = settings.bloom;
        jkGuiDisplay_aElements[23].selectedTextEntry = settings.ssao;
        jk_snwprintf(render_level, 255, u"%.2f", settings.ssaaMultiple);
    }
    jkGuiRend_SliderDraw(element, menu, vbuf, redraw);
    jkGuiRend_UpdateAndDrawClickable(&jkGuiDisplay_aElementsAdvanced[17], menu, 1);
}

void jkGuiDisplay_AnisotropyDraw(jkGuiElement *element, jkGuiMenu *menu, tVBuffer *vbuf, int redraw)
{
    int value = QualityPreset_AnisotropyFromSlider(jkGuiDisplay_aElementsAdvanced[19].selectedTextEntry);
    jk_snwprintf(anisotropy_val_text, 32, value == 1 ? u"Off" : u"%dx", value);
    jkGuiRend_SliderDraw(element, menu, vbuf, redraw);
    jkGuiRend_UpdateAndDrawClickable(&jkGuiDisplay_aElementsAdvanced[20], menu, 1);
}

int jkGuiDisplay_ShowAdvanced()
{
    int v0; // esi
    flex32_t parsedBias;

    jkGui_sub_412E20(&jkGuiDisplay_menuAdvanced, 100, 104, 100);
    jkGuiDisplay_aElementsAdvanced[9].selectedTextEntry = jkPlayer_bEnableJkgm;
    jkGuiDisplay_aElementsAdvanced[10].selectedTextEntry = jkPlayer_bEnableTexturePrecache;
    jkGuiDisplay_aElementsAdvanced[11].selectedTextEntry = jkPlayer_showFrameStats;
    jkGuiDisplay_aElementsAdvanced[16].selectedTextEntry = QualityPreset_Normalize(jkPlayer_qualityPreset);
    jkGuiDisplay_aElementsAdvanced[19].selectedTextEntry = QualityPreset_SliderFromAnisotropy(jkPlayer_anisotropy);
    jk_snwprintf(mipmap_bias_text, 32, u"%.2f", jkPlayer_mipmapBias);
    
    jkGuiRend_MenuSetReturnKeyShortcutElement(&jkGuiDisplay_menuAdvanced, &jkGuiDisplay_aElementsAdvanced[7]);
    jkGuiRend_MenuSetEscapeKeyShortcutElement(&jkGuiDisplay_menuAdvanced, &jkGuiDisplay_aElementsAdvanced[8]);
    jkGuiSetup_sub_412EF0(&jkGuiDisplay_menuAdvanced, 0);

    while (1)
    {
        v0 = jkGuiRend_DisplayAndReturnClicked(&jkGuiDisplay_menuAdvanced);

        if (v0 == GUI_SAFE_60)
        {
            jkPlayer_fpslimit = 60;
            jkPlayer_enableVsync = PRESENTATION_VSYNC_ON;
            jkGuiDisplay_aElements[18].selectedTextEntry = FrameRate_SliderFromValue(60);
            jkGuiDisplay_aElements[21].selectedTextEntry = PresentationMode_SliderFromVsync(PRESENTATION_VSYNC_ON);
            jkPlayer_WriteConf(jkPlayer_playerShortName);
            return 1;
        }

        if (v0 == GUI_RESET_VIDEO || v0 == GUI_SAFE_VIDEO)
        {
            const char* title = v0 == GUI_RESET_VIDEO ? "GUIEXT_RESET_VIDEO_TITLE" : "GUIEXT_SAFE_VIDEO_TITLE";
            const char* question = v0 == GUI_RESET_VIDEO ? "GUIEXT_RESET_VIDEO_Q" : "GUIEXT_SAFE_VIDEO_Q";
            if (jkGuiDialog_YesNoDialog(jkStrings_GetUniStringWithFallback(title),
                                        jkStrings_GetUniStringWithFallback(question)))
            {
                VideoDefaults defaults = v0 == GUI_RESET_VIDEO
                    ? video_defaults_recommended() : video_defaults_safe();
                if (jkGuiDisplay_ApplyDefaults(defaults))
                    return 1;
            }
            continue;
        }

        if (v0 == GUI_DIAGNOSTICS)
        {
            jkGuiDisplay_ShowDiagnostics();
            continue;
        }

        if ( v0 == 1 )
        {
            int preset = jkGuiDisplay_aElementsAdvanced[16].selectedTextEntry;
            jkPlayer_bEnableJkgm = jkGuiDisplay_aElementsAdvanced[9].selectedTextEntry;
            jkPlayer_bEnableTexturePrecache = jkGuiDisplay_aElementsAdvanced[10].selectedTextEntry;
            jkPlayer_showFrameStats = jkGuiDisplay_aElementsAdvanced[11].selectedTextEntry;
            jkPlayer_qualityPreset = QualityPreset_Normalize(preset);
            if (jkPlayer_qualityPreset != QUALITY_PRESET_CUSTOM)
            {
                QualityPresetSettings settings = QualityPreset_Get(jkPlayer_qualityPreset);
                jkPlayer_enableTextureFilter = settings.textureFiltering;
                jkPlayer_enableBloom = settings.bloom;
                jkPlayer_enableSSAO = settings.ssao;
                jkPlayer_anisotropy = settings.anisotropy;
                jkPlayer_mipmapBias = settings.mipmapBias;
                jkPlayer_ssaaMultiple = settings.ssaaMultiple;
                jkPlayer_bEnableTexturePrecache = settings.texturePrecache;
                jkPlayer_bEnableJkgm = settings.assetEnhancements;
                jkGuiDisplay_aElementsAdvanced[9].selectedTextEntry = jkPlayer_bEnableJkgm;
                jkGuiDisplay_aElementsAdvanced[10].selectedTextEntry = jkPlayer_bEnableTexturePrecache;
                jk_snwprintf(render_level, 255, u"%.2f", jkPlayer_ssaaMultiple);
            }
            else
            {
                jkPlayer_anisotropy = QualityPreset_AnisotropyFromSlider(jkGuiDisplay_aElementsAdvanced[19].selectedTextEntry);
                char biasText[32];
                stdString_WcharToChar(biasText, mipmap_bias_text, sizeof(biasText));
                if (_sscanf(biasText, "%f", &parsedBias) == 1)
                    jkPlayer_mipmapBias = parsedBias;
            }
            if (jkPlayer_mipmapBias < 0.25) jkPlayer_mipmapBias = 0.25;
            if (jkPlayer_mipmapBias > 4.0) jkPlayer_mipmapBias = 4.0;
            jkGuiDisplay_aElements[15].selectedTextEntry = jkPlayer_enableTextureFilter;
            jkGuiDisplay_aElements[22].selectedTextEntry = jkPlayer_enableBloom;
            jkGuiDisplay_aElements[23].selectedTextEntry = jkPlayer_enableSSAO;

            std3D_PurgeEntireTextureCache();
            std3D_UpdateSettings();

            jkPlayer_WriteConf(jkPlayer_playerShortName);
        }
        break;
    }
    return v0;
}

int jkGuiDisplay_Show()
{
    flex32_t ftmp;
    int v0; // esi

    jkGui_sub_412E20(&jkGuiDisplay_menu, 102, 104, 102);
    jkGuiRend_MenuSetReturnKeyShortcutElement(&jkGuiDisplay_menu, &jkGuiDisplay_aElements[7]);
    jkGuiRend_MenuSetEscapeKeyShortcutElement(&jkGuiDisplay_menu, &jkGuiDisplay_aElements[8]);
    jkGuiSetup_sub_412EF0(&jkGuiDisplay_menu, 0);

    jkGuiDisplay_aElements[10].selectedTextEntry = jkPlayer_fov - FOV_MIN;
    jkGuiDisplay_aElements[12].selectedTextEntry = jkPlayer_fovIsVertical;
    jkGuiDisplay_aElements[14].selectedTextEntry = Window_isHiDpi;
    jkGuiDisplay_aElements[15].selectedTextEntry = jkPlayer_enableTextureFilter;

    jkGuiDisplay_aElements[18].selectedTextEntry = FrameRate_SliderFromValue(jkPlayer_fpslimit);
    jkGuiDisplay_aElements[21].selectedTextEntry = PresentationMode_SliderFromVsync(jkPlayer_enableVsync);
    jkGuiDisplay_aElements[22].selectedTextEntry = jkPlayer_enableBloom;
    jkGuiDisplay_aElements[23].selectedTextEntry = jkPlayer_enableSSAO;

    ftmp = jkPlayer_ssaaMultiple;
    jk_snwprintf(render_level, 255, u"%.2f", ftmp);
    ftmp = jkPlayer_gamma;
    jk_snwprintf(gamma_level, 255, u"%.2f", ftmp);
    ftmp = jkPlayer_hudScale;
    jk_snwprintf(hud_level, 255, u"%.2f", ftmp);

continue_menu:
    v0 = jkGuiRend_DisplayAndReturnClicked(&jkGuiDisplay_menu);
    if (v0 == GUI_ADVANCED)
    {
        jkGuiDisplay_ShowAdvanced();
        goto continue_menu;
    }
    else if (v0 == GUI_ASPECT_OPTIONS)
    {
        jkGuiDisplay_ShowAspectOptions();
        goto continue_menu;
    }
    else if (v0 == GUI_DISPLAY_OPTIONS)
    {
        jkGuiDisplay_ShowDisplayOptions();
        goto continue_menu;
    }
    else if ( v0 != -1 )
    {
        DisplaySettings proposedDisplay = Window_GetDisplaySettings();
        proposedDisplay.hidpi = jkGuiDisplay_aElements[14].selectedTextEntry;

        jkPlayer_fov = FOV_MIN + jkGuiDisplay_aElements[10].selectedTextEntry;
        jkPlayer_fovIsVertical = jkGuiDisplay_aElements[12].selectedTextEntry;
        jkGuiDisplay_ApplyDisplayChange(proposedDisplay);
        jkPlayer_enableTextureFilter = jkGuiDisplay_aElements[15].selectedTextEntry;
        jkPlayer_fpslimit = FrameRate_ValueFromSlider(jkGuiDisplay_aElements[18].selectedTextEntry);
        jkPlayer_enableVsync = PresentationMode_VsyncFromSlider(jkGuiDisplay_aElements[21].selectedTextEntry);
        jkPlayer_enableBloom = jkGuiDisplay_aElements[22].selectedTextEntry;
        jkPlayer_enableSSAO = jkGuiDisplay_aElements[23].selectedTextEntry;
        char tmp[256];
        stdString_WcharToChar(tmp, render_level, 255);

        if(_sscanf(tmp, "%f", &ftmp) != 1) {
            jkPlayer_ssaaMultiple = 1.0;
        }
        else {
            jkPlayer_ssaaMultiple = QualityPreset_ClampSsaa(ftmp);
        }

        if (!QualityPreset_Matches(jkPlayer_qualityPreset,
                                   jkPlayer_enableTextureFilter,
                                   jkPlayer_anisotropy,
                                   jkPlayer_mipmapBias,
                                   jkPlayer_enableBloom,
                                   jkPlayer_enableSSAO,
                                   jkPlayer_ssaaMultiple,
                                   jkPlayer_bEnableTexturePrecache,
                                   jkPlayer_bEnableJkgm))
        {
            jkPlayer_qualityPreset = QUALITY_PRESET_CUSTOM;
        }

        stdString_WcharToChar(tmp, gamma_level, 255);
        if(_sscanf(tmp, "%f", &ftmp) != 1) {
            jkPlayer_gamma = 1.0;
        }
        else {
            jkPlayer_gamma = ftmp;
        }

        stdString_WcharToChar(tmp, hud_level, 255);
        if(_sscanf(tmp, "%f", &ftmp) != 1) {
            jkPlayer_hudScale = 1.0;
        }
        else {
            jkPlayer_hudScale = ftmp;
        }

        if (jkPlayer_hudScale > 100.0) {
            jkPlayer_hudScale = 100.0;
        }

        jkPlayer_WriteConf(jkPlayer_playerShortName);

        // Make sure filter settings get applied
        std3D_UpdateSettings();
    }
    return v0;
}

void jkGuiDisplay_sub_4149C0(){}
