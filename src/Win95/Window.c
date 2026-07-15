#include "Window.h"

#include "Win95/stdGdi.h"
#include "Platform/std3D.h"
#include "Main/Main.h"
#include "Main/jkMain.h"
#include "Main/jkGame.h"
#include "Gui/jkGUI.h"
#include "Gui/jkGUIRend.h"
#include "Win95/stdDisplay.h"
#include "World/jkPlayer.h"
#include "Platform/stdControl.h"
#include "stdPlatform.h"
#include "Devices/sithConsole.h"
#include "Platform/wuRegistry.h"
#include "Main/jkQuakeConsole.h"
#include "General/DiagnosticLog.h"
#include "General/DisplaySelection.h"
#include "General/DisplayMode.h"
#include "General/ResolutionLayout.h"
#include "General/AspectPolicy.h"
#include "General/FrameRate.h"
#include "General/FrameTelemetry.h"
#include "General/StoragePaths.h"
#include "General/PresentationMode.h"

#include "jk.h"

#ifdef _WIN32
#include <commctrl.h>
#endif

#ifdef ARCH_WASM
#include <emscripten.h>
#endif

#ifdef SDL2_RENDER

#include <fcntl.h> 
#include <stdio.h>
#ifndef _WIN32
#include <unistd.h>
#endif //!_WIN32

#if !defined(WIN64_MINGW) && !defined(_WIN32)
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#else
#include <conio.h>
#endif
//#include <stropts.h>

#include "SDL2_helper.h"

#include <string.h>

//#include <GL/glew.h>
#ifndef MACOS
//#include <GL/gl.h>
#endif
#include "Win95/Video.h"

#if defined(MACOS)
#include <stdbool.h>
#import <Carbon/Carbon.h>
#endif

extern int Window_xPos, Window_yPos;
#endif // SDL2_RENDER

int Window_xSize = WINDOW_DEFAULT_WIDTH;
int Window_ySize = WINDOW_DEFAULT_HEIGHT;
int Window_screenXSize = WINDOW_DEFAULT_WIDTH;
int Window_screenYSize = WINDOW_DEFAULT_HEIGHT;
int Window_isHiDpi = 0;
int Window_isFullscreen = 0;
DisplayMode Window_displayMode = DISPLAY_MODE_BORDERLESS;
int Window_needsRecreate = 0;
int Window_bShouldPopSteamKeyboard = 0;
static int Window_bSafeMode = 0;
static int Window_bRestorationGuardReady = 0;
static int Window_displayMonitor = 0;
static int Window_windowWidth = WINDOW_DEFAULT_WIDTH;
static int Window_windowHeight = WINDOW_DEFAULT_HEIGHT;
static int Window_requestedRefreshHz = 0;
static int Window_bDeferDisplayPersistence = 0;
static PresentationVsyncMode Window_appliedVsync = PRESENTATION_VSYNC_OFF;
static int Window_contextFallbackTier = 0;
static char Window_rendererVendor[256] = "";
static char Window_rendererGpu[256] = "";
static char Window_rendererDriver[256] = "";
static char Window_rendererApi[256] = "";
static char Window_rendererFrameCap[32] = "";
static char Window_rendererVsync[64] = "";
static int Window_bSwapLogged = 0;

void Window_SetSafeMode(int enabled)
{
    Window_bSafeMode = enabled != 0;
}

int Window_IsSafeMode(void)
{
    return Window_bSafeMode;
}

void Window_SetHiDpi(int val)
{
    if (Window_isHiDpi != val)
    {
        Window_isHiDpi = val;

        Window_needsRecreate = 1;
    }

    if (!Window_bDeferDisplayPersistence)
        wuRegistry_SaveBool("Window_isHiDpi", Window_isHiDpi);
}

void Window_SetFullscreen(int val)
{
    Window_SetDisplayMode(val ? DISPLAY_MODE_BORDERLESS : DISPLAY_MODE_WINDOWED);
}

void Window_SetDisplayMode(DisplayMode mode)
{
    DisplayMode resolved = display_mode_resolve(mode, Window_bSafeMode != 0, Window_bRestorationGuardReady != 0);
    int fullscreen = resolved != DISPLAY_MODE_WINDOWED;
    if (Window_displayMode != resolved)
    {
        // Reset window when exiting fullscreen
        // TODO: Add settings for these sizes maybe?
        if (Window_isFullscreen && !fullscreen) {
            Window_xSize = WINDOW_DEFAULT_WIDTH;
            Window_ySize = WINDOW_DEFAULT_HEIGHT;
            Window_screenXSize = WINDOW_DEFAULT_WIDTH;
            Window_screenYSize = WINDOW_DEFAULT_HEIGHT;
#ifdef SDL2_RENDER
            Window_xPos = SDL_WINDOWPOS_CENTERED;
            Window_yPos = SDL_WINDOWPOS_CENTERED;
#endif
        }

        Window_displayMode = resolved;
        Window_isFullscreen = fullscreen;
        Window_needsRecreate = 1;
    }

    if (!Window_bDeferDisplayPersistence)
    {
        wuRegistry_SaveBool("Window_isFullscreen", Window_isFullscreen);
        wuRegistry_SaveInt("Window_displayMode", (int)Window_displayMode);
    }

    {
        char event[96];
        snprintf(event, sizeof(event), "display_mode_selected mode=%s requested=%s guard_ready=%s",
                 display_mode_name(resolved), display_mode_name(mode), Window_bRestorationGuardReady ? "true" : "false");
        diag_log_event(DIAG_SEVERITY_INFO, "display", event);
    }
}

#ifdef _WIN32
static HRESULT CALLBACK Window_DisplayConfirmCallback(HWND dialog, UINT notification, WPARAM timerMs, LPARAM unused, LONG_PTR timeoutMs)
{
    (void)unused;
    if (notification == TDN_TIMER && timerMs >= (WPARAM)timeoutMs)
        SendMessageW(dialog, TDM_CLICK_BUTTON, IDCANCEL, 0);
    return S_OK;
}
#endif

int Window_ConfirmDisplaySettings(unsigned int timeoutMs)
{
#ifdef _WIN32
    const TASKDIALOG_BUTTON buttons[] = {
        { IDYES, L"Keep changes" },
        { IDCANCEL, L"Revert" }
    };
    TASKDIALOGCONFIG config;
    int selected = IDCANCEL;
    memset(&config, 0, sizeof(config));
    config.cbSize = sizeof(config);
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_CALLBACK_TIMER | TDF_POSITION_RELATIVE_TO_WINDOW;
    config.pszWindowTitle = L"OpenJKDF2 AMD Enhanced";
    config.pszMainInstruction = L"Keep these display settings?";
    config.pszContent = L"The previous display settings will be restored automatically in 15 seconds.";
    config.pszMainIcon = TD_WARNING_ICON;
    config.cButtons = (UINT)(sizeof(buttons) / sizeof(buttons[0]));
    config.pButtons = buttons;
    config.nDefaultButton = IDCANCEL;
    config.pfCallback = Window_DisplayConfirmCallback;
    config.lpCallbackData = (LONG_PTR)timeoutMs;
    if (FAILED(TaskDialogIndirect(&config, &selected, NULL, NULL)))
        return 0;
    return selected == IDYES;
#else
    (void)timeoutMs;
    return 0;
#endif
}

//static wm_handler Window_ext_handlers[16] = {0};

int Window_AddMsgHandler(WindowHandler_t a1)
{
    int i = 0;

    // Added: no duplicates
    for (i = 0; i < 16; i++)
    {
        if (Window_ext_handlers[i].exists && Window_ext_handlers[i].handler == a1)
            return 1;
    }

    for (i = 0; i < 16; i++)
    {
        if ( !Window_ext_handlers[i].exists )
            break;
    }
    
    // Added: no OOB
    if (i >= 16) return 1;

    Window_ext_handlers[i].handler = a1;
    Window_ext_handlers[i].exists = 1;
    ++g_handler_count;
    return 1;
}

int Window_RemoveMsgHandler(WindowHandler_t a1)
{
    int i = 0;

    // Added: the original would still decrement on missing handlers
    for (i = 0; i < 16; i++)
    {
        if ( Window_ext_handlers[i].handler == a1 )
        {
            Window_ext_handlers[i].handler = 0;
            Window_ext_handlers[i].exists = 0;
            g_handler_count -= 1; // doing g_handler_count-- changes behavior???
            return 1;
        }
    }

    return 1;
}

int Window_AddDialogHwnd(HWND a1)
{
    int v1; // eax

    v1 = g_thing_two_some_dialog_count;
    if ( (unsigned int)g_thing_two_some_dialog_count >= 0x10 )
        return 0;
    Window_aDialogHwnds[g_thing_two_some_dialog_count] = a1;
    g_thing_two_some_dialog_count = v1 + 1;
    return 1;
}

#if !defined(SDL2_RENDER) && defined(WIN32)
#define dword_855E98 (*(int*)0x855E98)
#define dword_855DE4 (*(int*)0x855DE4)
#else
static int dword_855E98 = 0;
static int dword_855DE4 = 0;
#endif // SDL2_RENDER

int Window_msg_main_handler(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    int handler_count; // ebx
    struct wm_handler *ext_handler; // esi
    DWORD dwProcessId; // [esp+10h] [ebp-8h] BYREF
    LRESULT v10; // [esp+14h] [ebp-4h] BYREF

    switch ( Msg )
    {
        case WM_CREATE:
            g_app_active = 0;
            g_window_active = 0;
            break;
        case WM_DESTROY:
            g_window_not_destroyed = 0;
            Main_Shutdown();
            break;
        case WM_ACTIVATE:
            if ( (uint16_t)wParam == 2 || (uint16_t)wParam == 1 )// WA_ACTIVE or WA_CLICKACTIVE
            {
                g_window_active = 1;
                if ( dword_855E98 )
                {
                    dword_855E98 = 0;
                    if ( Window_setCooperativeLevel )
                        Window_setCooperativeLevel(0);
                }
#ifdef WIN32_BLOBS
                jk_SetFocus(g_hWnd);
#endif
            }
            else
            {
                if ( dword_855DE4 == 1 && g_window_not_destroyed && g_app_active && !dword_855E98 )
                {
                    dwProcessId = 0;
                    lParam = 1;
                    if ( lParam )
                    {
#ifdef WIN32_BLOBS
                        jk_GetWindowThreadProcessId((HWND)lParam, (LPDWORD)&lParam);
                        jk_GetWindowThreadProcessId(hWnd, &dwProcessId);
#endif
                    }
                    if ( dwProcessId == lParam )
                    {
                        dword_855E98 = 1;
                        if ( Window_drawAndFlip )
                            Window_drawAndFlip(0);
                    }
                }
                g_window_active = 0;
            }
            break;
        case WM_ACTIVATEAPP:
            g_app_active = wParam != 0;
            break;
        default:
            break;
    }

    if ( !g_app_active || (g_app_suspended = 1, !g_window_active) )
        g_app_suspended = 0;
    handler_count = 0;

    if ( g_handler_count <= 0 )
        return Window_DefaultHandler(hWnd, Msg, wParam, lParam, NULL);

    for ( ext_handler = Window_ext_handlers; !ext_handler->exists || !ext_handler->handler(hWnd, Msg, wParam, lParam, &v10); ++ext_handler )
    {
        if ( ++handler_count >= g_handler_count )
            return Window_DefaultHandler(hWnd, Msg, wParam, lParam, NULL);
    }
    return v10;
}

#if !defined(SDL2_RENDER) && defined(WIN32)

int Window_Main(HINSTANCE hInstance, int a2, char *lpCmdLine, int nShowCmd, LPCSTR lpWindowName)
{
    int result;
    WNDCLASSEXA wndClass;
    MSG msg;

    g_handler_count = 0;
    g_thing_two_some_dialog_count = 0;
    g_should_exit = 0;
    g_window_not_destroyed = 0;
    g_hInstance = hInstance;
    g_nShowCmd = nShowCmd;

    wndClass.cbSize = 48;
    wndClass.hInstance = hInstance;
    wndClass.lpszClassName = "wKernel";
    wndClass.lpszMenuName = 0;
    wndClass.lpfnWndProc = Window_msg_main_handler;
    wndClass.style = 3;
    wndClass.hIcon = jk_LoadIconA(hInstance, "APPICON");
    if ( !wndClass.hIcon )
        wndClass.hIcon = jk_LoadIconA(0, (void*)32512);
    wndClass.hIconSm = jk_LoadIconA(hInstance, "APPICON");
    if ( !wndClass.hIconSm )
        wndClass.hIconSm = jk_LoadIconA(0, (void*)32512);
    wndClass.hCursor = jk_LoadCursorA(0, (void*)0x7F00);
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hbrBackground = jk_GetStockObject(4);

    if (jk_RegisterClassExA(&wndClass))
    {
        if ( jk_FindWindowA("wKernel", lpWindowName) )
            jk_exit(-1);

        uint32_t hres = jk_GetSystemMetrics(1);
        uint32_t vres = jk_GetSystemMetrics(0);
        g_hWnd = jk_CreateWindowExA(0x40000u, "wKernel", lpWindowName, 0x90000000, 0, 0, vres, hres, 0, 0, hInstance, 0);

        if (g_hWnd)
        {
            g_hInstance = hInstance;
            jk_ShowWindow(g_hWnd, 1);
            jk_UpdateWindow(g_hWnd);
        }
    }

    stdGdi_SetHwnd(g_hWnd);
    stdGdi_SetHInstance(g_hInstance);
    jk_InitCommonControls();

    g_855E8C = 2 * jk_GetSystemMetrics(32);
    uint32_t metrics_32 = jk_GetSystemMetrics(32);
    g_855E90 = jk_GetSystemMetrics(15) + 2 * metrics_32;
    result = Main_Startup(lpCmdLine);

    if (!result) return result;

    
    g_window_not_destroyed = 1;

    while (1)
    {
        if (jk_PeekMessageA(&msg, 0, 0, 0, 0))
        {
            if (!jk_GetMessageA(&msg, 0, 0, 0))
            {
                result = msg.wParam;
                g_should_exit = 1;
                break;
            }

            uint32_t some_cnt = 0;
            if (g_thing_two_some_dialog_count > 0)
            {
#if 0
                v16 = &thing_three;
                do
                {
                    //TODO if ( jk_IsDialogMessageA(*v16, &msg) )
                    //  break;
                    ++some_cnt;
                    ++v16;
                }
                while ( some_cnt < g_thing_two_some_dialog_count );
#endif
            }

            if (some_cnt == g_thing_two_some_dialog_count)
            {
                jk_TranslateMessage(&msg);
                jk_DispatchMessageA(&msg);
            }

            if (!jk_PeekMessageA(&msg, 0, 0, 0, 0))
            {
                result = 0;
                if ( g_should_exit )
                    return result;
            }
        }

        //if (user32->stopping) break;

        jkMain_GuiAdvance();
    }

    return result;
}

int Window_DefaultHandler(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, void* unused)
{
    return DefWindowProcA(hWnd, Msg, wParam, lParam);
}

#endif

#ifdef SDL2_RENDER

SDL_Window* displayWindow = NULL;
SDL_Event event;
SDL_GLContext glWindowContext;

int Window_lastXRel = 0;
int Window_lastYRel = 0;
int Window_lastSampleTime = 0;
static PresentationVsyncMode Window_ApplyVsyncMode(int requestedMode)
{
    PresentationVsyncMode mode = PresentationMode_NormalizeVsync(requestedMode);
    char event[128];

    if (SDL_GL_SetSwapInterval((int)mode))
    {
        Window_appliedVsync = mode;
        return mode;
    }

    if (mode == PRESENTATION_VSYNC_ADAPTIVE && SDL_GL_SetSwapInterval(PRESENTATION_VSYNC_ON))
    {
        diag_log_event(DIAG_SEVERITY_WARNING, "presentation", "vsync=adaptive unsupported fallback=on");
        Window_appliedVsync = PRESENTATION_VSYNC_ON;
        return Window_appliedVsync;
    }

    SDL_GL_SetSwapInterval(PRESENTATION_VSYNC_OFF);
    Window_appliedVsync = PRESENTATION_VSYNC_OFF;
    snprintf(event, sizeof(event), "vsync=%s apply_failed fallback=off", PresentationMode_VsyncName(mode));
    diag_log_event(DIAG_SEVERITY_WARNING, "presentation", event);
    return Window_appliedVsync;
}

static void Window_CopyRendererString(char* output, size_t outputSize, GLenum name)
{
    const GLubyte* value = glGetString(name);
    snprintf(output, outputSize, "%s", value ? (const char*)value : "Not collected");
}

static int Window_CurrentRefreshRate(void)
{
    const SDL_DisplayMode* mode = NULL;
    SDL_DisplayID display = displayWindow ? SDL_GetDisplayForWindow(displayWindow) : SDL_GetPrimaryDisplay();
    if (display)
        mode = SDL_GetCurrentDisplayMode(display);
    if (!mode && display)
        mode = SDL_GetDesktopDisplayMode(display);
    return mode ? (int)(mode->refresh_rate + 0.5f) : 0;
}

void Window_GetRendererDiagnostics(RendererDiagnostics* diagnostics)
{
    const char* fallback = "Primary core profile";
    if (!diagnostics)
        return;
    if (Window_contextFallbackTier == 1)
        fallback = "OpenGL 3.3 core fallback";
    else if (Window_contextFallbackTier == 2)
        fallback = "OpenGL 3.2 core fallback";

    if (jkPlayer_fpslimit == FRAME_RATE_DESKTOP_REFRESH)
        snprintf(Window_rendererFrameCap, sizeof(Window_rendererFrameCap), "Desktop Refresh");
    else if (jkPlayer_fpslimit == FRAME_RATE_UNLIMITED)
        snprintf(Window_rendererFrameCap, sizeof(Window_rendererFrameCap), "Unlimited");
    else
        snprintf(Window_rendererFrameCap, sizeof(Window_rendererFrameCap), "%d FPS", jkPlayer_fpslimit);

    if (jkPlayer_enableVsync == PRESENTATION_VSYNC_ADAPTIVE && Window_appliedVsync == PRESENTATION_VSYNC_ON)
        snprintf(Window_rendererVsync, sizeof(Window_rendererVsync), "On (adaptive unsupported)");
    else
        snprintf(Window_rendererVsync, sizeof(Window_rendererVsync), "%s", PresentationMode_VsyncName(Window_appliedVsync));

    diagnostics->backend = "OpenGL Core";
    diagnostics->gpu = Window_rendererGpu;
    diagnostics->vendor = Window_rendererVendor;
    diagnostics->driver = Window_rendererDriver;
    diagnostics->api = Window_rendererApi;
    diagnostics->fallback = fallback;
    diagnostics->vsync = Window_rendererVsync;
    diagnostics->display_mode = display_mode_name(Window_displayMode);
    diagnostics->width = Window_xSize;
    diagnostics->height = Window_ySize;
    diagnostics->refresh_hz = Window_CurrentRefreshRate();
    diagnostics->frame_cap = Window_rendererFrameCap;
}

int Window_GetAppliedVsyncMode(void)
{
    return (int)Window_appliedVsync;
}

int Window_lastSampleMs = 0;
int Window_bMouseLeft = 0;
int Window_bMouseRight = 0;
int Window_resized = 0;
int Window_mouseX = 0;
int Window_mouseY = 0;
int Window_mouseWheelX = 0;
int Window_mouseWheelY = 0;
uint64_t Window_validationMouseEventNs = 0;
uint64_t Window_validationMouseHandlerNs = 0;
unsigned int Window_validationMouseSequence = 0;
unsigned int Window_validationMouseConsumedSequence = 0;
int Window_lastMouseX = 0;
int Window_lastMouseY = 0;
static int Window_mouseCaptureActive = 0;
static int Window_mouseRelativeActive = 0;
static int Window_mouseCaptureRawSetting = -1;
static int Window_mouseCaptureAccelerationSetting = -1;

static void Window_SetGameplayMouseCapture(int enabled)
{
    int rawSetting = jkPlayer_rawMouseInput != 0;
    int accelerationSetting = jkPlayer_mouseAcceleration != 0;
    int settingsChanged = rawSetting != Window_mouseCaptureRawSetting ||
        accelerationSetting != Window_mouseCaptureAccelerationSetting;

    if (!displayWindow)
        return;
    if (!!enabled == Window_mouseCaptureActive && (!enabled || !settingsChanged))
        return;

    if (!enabled)
    {
        SDL_SetWindowRelativeMouseMode(displayWindow, false);
        SDL_SetWindowMouseGrab(displayWindow, false);
        SDL_ShowCursor();
        Window_lastXRel = 0;
        Window_lastYRel = 0;
        if (Window_mouseCaptureActive)
            diag_log_event(DIAG_SEVERITY_INFO, "input", "mouse_capture=released");
        Window_mouseCaptureActive = 0;
        Window_mouseRelativeActive = 0;
        return;
    }

    if (Window_mouseCaptureActive)
        Window_SetGameplayMouseCapture(0);

    SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_MODE_CENTER, "1");
    SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE, accelerationSetting ? "1" : "0");
    Window_mouseCaptureRawSetting = rawSetting;
    Window_mouseCaptureAccelerationSetting = accelerationSetting;

    if (rawSetting && SDL_SetWindowRelativeMouseMode(displayWindow, true))
    {
        Window_mouseRelativeActive = 1;
        diag_log_event(DIAG_SEVERITY_INFO, "input",
            accelerationSetting ? "mouse_backend=SDL_relative raw_preferred=true acceleration=system" :
                                  "mouse_backend=SDL_relative raw_preferred=true acceleration=off");
    }
    else
    {
        SDL_SetWindowRelativeMouseMode(displayWindow, false);
        SDL_SetWindowMouseGrab(displayWindow, true);
        SDL_HideCursor();
        Window_mouseRelativeActive = 0;
        diag_log_event(rawSetting ? DIAG_SEVERITY_WARNING : DIAG_SEVERITY_INFO, "input",
            rawSetting ? "mouse_backend=grabbed_fallback raw_relative_failed=true" :
                         "mouse_backend=grabbed_fallback raw_preferred=false");
    }
    Window_mouseCaptureActive = 1;
}

int Window_xPos = SDL_WINDOWPOS_CENTERED;
int Window_yPos = SDL_WINDOWPOS_CENTERED;

static SDL_DisplayID Window_DisplayIdForOrdinal(int monitor)
{
    SDL_DisplayID result = 0;
    int count = 0;
    SDL_DisplayID* displays = SDL_GetDisplays(&count);
    if (displays && monitor >= 0 && monitor < count)
        result = displays[monitor];
    if (displays) SDL_free(displays);
    return result ? result : SDL_GetPrimaryDisplay();
}

int Window_GetDisplayInventory(DisplayInventory* out_inventory)
{
    SDL_DisplayID* displays;
    SDL_DisplayID primary;
    int count = 0;
    int index;
    if (!out_inventory) return 0;
    memset(out_inventory, 0, sizeof(*out_inventory));
    displays = SDL_GetDisplays(&count);
    primary = SDL_GetPrimaryDisplay();
    if (!displays || count <= 0)
    {
        const SDL_DisplayMode* desktop = primary ? SDL_GetDesktopDisplayMode(primary) : NULL;
        if (displays) SDL_free(displays);
        if (!desktop) return 0;
        out_inventory->monitor_count = 1;
        out_inventory->monitors[0].desktop_width = desktop->w;
        out_inventory->monitors[0].desktop_height = desktop->h;
        out_inventory->monitors[0].desktop_refresh_hz = (int)(desktop->refresh_rate + 0.5f);
        return 1;
    }
    if (count > DISPLAY_SELECTION_MAX_MONITORS) count = DISPLAY_SELECTION_MAX_MONITORS;
    out_inventory->monitor_count = count;
    for (index = 0; index < count; ++index)
    {
        DisplayMonitor* target = &out_inventory->monitors[index];
        const SDL_DisplayMode* desktop = SDL_GetDesktopDisplayMode(displays[index]);
        SDL_DisplayMode** modes;
        int mode_count = 0;
        int mode_index;
        if (displays[index] == primary) out_inventory->primary_monitor = index;
        if (desktop)
        {
            target->desktop_width = desktop->w;
            target->desktop_height = desktop->h;
            target->desktop_refresh_hz = (int)(desktop->refresh_rate + 0.5f);
        }
        modes = SDL_GetFullscreenDisplayModes(displays[index], &mode_count);
        for (mode_index = 0; modes && mode_index < mode_count &&
             target->mode_count < DISPLAY_SELECTION_MAX_MODES; ++mode_index)
        {
            const SDL_DisplayMode* mode = modes[mode_index];
            DisplayResolution candidate;
            int duplicate = 0;
            int existing;
            if (!mode || mode->w <= 0 || mode->h <= 0) continue;
            candidate.width = mode->w;
            candidate.height = mode->h;
            candidate.refresh_hz = (int)(mode->refresh_rate + 0.5f);
            for (existing = 0; existing < target->mode_count; ++existing)
            {
                const DisplayResolution* current = &target->modes[existing];
                if (current->width == candidate.width &&
                    current->height == candidate.height &&
                    current->refresh_hz == candidate.refresh_hz)
                {
                    duplicate = 1;
                    break;
                }
            }
            if (!duplicate) target->modes[target->mode_count++] = candidate;
        }
        if (modes) SDL_free(modes);
    }
    SDL_free(displays);
    return 1;
}

const char* Window_GetDisplayName(int monitor)
{
    SDL_DisplayID display = Window_DisplayIdForOrdinal(monitor);
    const char* name = display ? SDL_GetDisplayName(display) : NULL;
    return name ? name : "Display";
}

DisplaySettings Window_GetDisplaySettings(void)
{
    DisplaySettings settings = {
        Window_displayMode, Window_displayMonitor, Window_screenXSize,
        Window_screenYSize,
        Window_displayMode == DISPLAY_MODE_WINDOWED ? 0 : Window_CurrentRefreshRate(),
        Window_isHiDpi
    };
    if (Window_displayMode == DISPLAY_MODE_WINDOWED)
    {
        settings.width = Window_windowWidth;
        settings.height = Window_windowHeight;
    }
    return settings;
}

int Window_ApplyDisplaySettings(DisplaySettings requested, DisplaySelectionReason* reason)
{
    DisplayInventory inventory;
    DisplaySelectionResult result;
    if (!Window_GetDisplayInventory(&inventory))
    {
        if (reason) *reason = DISPLAY_SELECTION_NO_DISPLAYS;
        return 0;
    }
    result = display_selection_resolve(&inventory, requested, Window_bRestorationGuardReady);
    if (reason) *reason = result.reason;
    if (!result.accepted) return 0;
    Window_displayMonitor = result.settings.monitor;
    Window_requestedRefreshHz = result.settings.refresh_hz;
    if (result.settings.mode == DISPLAY_MODE_WINDOWED)
    {
        Window_windowWidth = result.settings.width;
        Window_windowHeight = result.settings.height;
    }
    Window_bDeferDisplayPersistence = 1;
    Window_SetDisplayMode(result.settings.mode);
    Window_SetHiDpi(result.settings.hidpi);
    Window_bDeferDisplayPersistence = 0;
    Window_screenXSize = result.settings.width;
    Window_screenYSize = result.settings.height;
    Window_xSize = result.settings.width;
    Window_ySize = result.settings.height;
    Window_needsRecreate = 1;
    return 1;
}

void Window_CommitDisplaySettings(DisplaySettings settings)
{
    Window_displayMonitor = settings.monitor;
    if (settings.mode == DISPLAY_MODE_WINDOWED)
    {
        Window_windowWidth = settings.width;
        Window_windowHeight = settings.height;
    }
    Window_requestedRefreshHz = settings.refresh_hz;
    wuRegistry_SaveBool("Window_isFullscreen", settings.mode != DISPLAY_MODE_WINDOWED);
    wuRegistry_SaveInt("Window_displayMode", (int)settings.mode);
    wuRegistry_SaveBool("Window_isHiDpi", settings.hidpi);
    wuRegistry_SaveInt("Window_displayMonitor", settings.monitor);
    wuRegistry_SaveInt("Window_windowWidth", Window_windowWidth);
    wuRegistry_SaveInt("Window_windowHeight", Window_windowHeight);
    wuRegistry_SaveInt("Window_refreshHz", settings.refresh_hz);
}

int Window_IsRestorationGuardReady(void)
{
    return Window_bRestorationGuardReady;
}

void Window_SetRestorationGuardReady(int ready)
{
    Window_bRestorationGuardReady = ready != 0;
}

int last_jkGame_isDDraw = 0;
#ifdef QUAKE_CONSOLE
int last_jkQuakeConsole_bOpen = 0;
#endif
int Window_menu_mouseX = 0;
int Window_menu_mouseY = 0;

extern int jkGuiBuildMulti_bRendering;

void Window_HandleMouseMove(SDL_MouseMotionEvent *event)
{
    int x = (int)event->x;
    int y = (int)event->y;

    Window_lastMouseX = Window_mouseX;
    Window_lastMouseY = Window_mouseY;

    if (!jkGame_isDDraw)
    {
        ResolutionLayoutRect menuViewport = AspectPolicy_Destination(
            Window_xSize, Window_ySize, 640, 480, jkPlayer_preserveMenuAspect);
        double logicalX;
        double logicalY;

        ResolutionLayout_MapPoint(
            &menuViewport, (double)x, (double)y, 640.0, 480.0, 1, &logicalX, &logicalY);
        Window_mouseX = (int)logicalX;
        Window_mouseY = (int)logicalY;
    }
    else
    {
        Window_mouseX = x;
        Window_mouseY = y;// - (Window_ySize - 480);
    }

    if (Window_mouseX < 0)
        Window_mouseX = 0;

    if (jkQuakeConsole_bOpen) return; // Hijack all input to console

    uint32_t pos = ((Window_mouseX) & 0xFFFF) | (((Window_mouseY) << 16) & 0xFFFF0000);
    
    // event->timestamp is nanoseconds as of SDL3 (was milliseconds); convert to
    // milliseconds to keep Window_lastSampleTime's units (set from SDL_GetTicks()).
    Window_lastSampleMs = (int)(event->timestamp / 1000000) - Window_lastSampleTime;
    //Window_lastSampleTime = event->timestamp;
    Window_lastXRel += (int)event->xrel;
    Window_lastYRel += (int)event->yrel;

    Window_msg_main_handler(g_hWnd, WM_MOUSEMOVE, 0, pos);
}

int jkCutscene_wasPaused = 0;
int jkGame_wasDDraw = 0;
int Window_bNeedsKeyboardFixed = 0;
void Window_HandleWindowEvent(SDL_Event* event)
{
    // SDL3 flattened window sub-events into top-level SDL_EVENT_WINDOW_* types
    // (no more nested event->window.event); this dispatches on event->type,
    // called for the whole SDL_EVENT_WINDOW_FIRST..LAST range.
    switch (event->type)
    {
        case SDL_EVENT_WINDOW_SHOWN:
#ifdef MACOS
            {
                static int bMacosOnlyOncePerProcessLifetimeTriggerTheStupidDylibLoad = 0;
                if (!bMacosOnlyOncePerProcessLifetimeTriggerTheStupidDylibLoad)
                {
                    CGEventRef ref = CGEventCreateKeyboardEvent(NULL, 0x72 /* help */, 1);
                    CGEventSetFlags( ref, kCGEventFlagMaskNumericPad );
                    CGEventSetFlags( ref, kCGEventFlagMaskSecondaryFn );
                    CGEventPost(kCGHIDEventTap, ref);
                    CFRelease(ref);
                    bMacosOnlyOncePerProcessLifetimeTriggerTheStupidDylibLoad = 1;
                }
            }
#endif
            //printf("Window %d shown", event->window.windowID);
            break;
        case SDL_EVENT_WINDOW_HIDDEN:
            //printf("Window %d hidden", event->window.windowID);
            break;
        case SDL_EVENT_WINDOW_EXPOSED:
            //printf("Window %d exposed", event->window.windowID);
            break;
        case SDL_EVENT_WINDOW_MOVED:
            /*printf("Window %d moved to %d,%d",
                    event->window.windowID, event->window.data1,
                    event->window.data2);*/
            Window_xPos = event->window.data1;
            Window_yPos = event->window.data2;
            break;
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            if (Window_xSize != event->window.data1 || Window_ySize != event->window.data2)
                Window_resized = 1;

            //Window_xSize = event->window.data1;
            //Window_ySize = event->window.data2;
            SDL_GetWindowSizeInPixels(displayWindow, &Window_xSize, &Window_ySize);
            SDL_GetWindowSize(displayWindow, &Window_screenXSize, &Window_screenYSize);

            if (Window_xSize < 640) Window_xSize = 640;
            if (Window_ySize < 480) Window_ySize = 480;
            //printf("%u %u\n", Window_xSize, Window_ySize);
            break;
        case SDL_EVENT_WINDOW_MINIMIZED:
            stdPlatform_Printf("Window %d minimized", event->window.windowID);

            // HACK: Cutscene audio gets messed up when multitasking on Android :/
#ifdef TARGET_ANDROID
            stdPlatform_Printf("SDL_EVENT_WINDOW_MINIMIZED");
            jkCutscene_wasPaused = jkCutscene_55AA54 && jkCutscene_isRendering && std3D_IsReady();
            jkGame_wasDDraw = jkGame_isDDraw;
            if (std3D_IsReady() && jkCutscene_isRendering && !jkCutscene_wasPaused) {
                stdPlatform_Printf("Pause cutscene...\n");
                Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_SPACE, 0);
                Window_msg_main_handler(g_hWnd, WM_CHAR, VK_SPACE, 0);
            }
            else if (std3D_IsReady() && jkGame_isDDraw) {
                stdPlatform_Printf("Pause game...\n");
                Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_ESCAPE, 0);
                Window_msg_main_handler(g_hWnd, WM_CHAR, VK_ESCAPE, 0);
            }
#endif
            break;
        case SDL_EVENT_WINDOW_MAXIMIZED:
            stdPlatform_Printf("Window %d maximized", event->window.windowID);
            break;
        case SDL_EVENT_WINDOW_RESTORED:
            stdPlatform_Printf("Window %d restored", event->window.windowID);

            // HACK: Cutscene audio gets messed up when multitasking on Android :/
#ifdef TARGET_ANDROID
            stdPlatform_Printf("SDL_EVENT_WINDOW_RESTORED");
            if (std3D_IsReady() && jkCutscene_isRendering && !jkCutscene_wasPaused) {
                stdPlatform_Printf("Play cutscene...\n");
                Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_SPACE, 0);
                Window_msg_main_handler(g_hWnd, WM_CHAR, VK_SPACE, 0);
            }
            else if (std3D_IsReady() && !jkGame_isDDraw && jkGame_wasDDraw) {
                stdPlatform_Printf("Resume game...\n");
                Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_ESCAPE, 0);
                Window_msg_main_handler(g_hWnd, WM_CHAR, VK_ESCAPE, 0);
            }
#endif
            break;
        case SDL_EVENT_WINDOW_MOUSE_ENTER:
            stdPlatform_Printf("Mouse entered window %d\n", event->window.windowID);
            break;
        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
            stdPlatform_Printf("Mouse left window %d\n", event->window.windowID);
            break;
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
            stdPlatform_Printf("Window %d gained keyboard focus\n", event->window.windowID);
            diag_log_event(DIAG_SEVERITY_INFO, "display", "window_focus_gained");
            Window_bNeedsKeyboardFixed = 0;
            break;
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            stdPlatform_Printf("Window %d lost keyboard focus\n", event->window.windowID);
            diag_log_event(DIAG_SEVERITY_INFO, "display", "window_focus_lost");
            Window_SetGameplayMouseCapture(0);
            if (stdControl_IsSystemKeyboardShowing() && Window_bNeedsKeyboardFixed) {
                stdPlatform_Printf("Fixing keyboard...\n");

                SDL_Window* gimmeKeyboard = SDL_CreateWindow("Gimme Keyboard", 20, 20, SDL_WINDOW_KEYBOARD_GRABBED | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS);
                SDL_RaiseWindow(gimmeKeyboard);
                SDL_DestroyWindow(gimmeKeyboard);
                SDL_RaiseWindow(displayWindow);

                SDL_MinimizeWindow(displayWindow);
                SDL_RestoreWindow(displayWindow);
                SDL_RaiseWindow(displayWindow);
            }
            break;
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            //printf("Window %d closed", event->window.windowID);
            break;
        case SDL_EVENT_WINDOW_HIT_TEST:
            //printf("Window %d has a special hit test", event->window.windowID);
            break;
    }
}

#if defined(WIN64_MINGW) || defined(_WIN32)
CHAR my_getch() {
    DWORD mode, cc;
    DWORD num;
    INPUT_RECORD irInBuf[1];
    HANDLE h = GetStdHandle( STD_INPUT_HANDLE );

    if (h == NULL) {
        return 0; // console not found
    }

    GetConsoleMode( h, &mode );
    SetConsoleMode( h, mode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT) );
    TCHAR c = 0;
    GetNumberOfConsoleInputEvents(h, &num);
    if (num)
    {
        if (!ReadConsoleInput(
            h,      // input buffer handle 
            irInBuf,     // buffer to read into 
            1,         // size of read buffer 
            &num))
        {

        }
        else
        {
            if (irInBuf[0].EventType == KEY_EVENT && irInBuf[0].Event.KeyEvent.bKeyDown) {
                c = irInBuf[0].Event.KeyEvent.uChar.AsciiChar;
            }
        }
    }
    SetConsoleMode( h, mode );
    return c;
}

int my_kbhit() {
    DWORD num;
    DWORD mode, cc;
    HANDLE h = GetStdHandle( STD_INPUT_HANDLE );
    if (h == NULL) {
        return 0; // console not found
    }

    GetConsoleMode( h, &mode );
    SetConsoleMode( h, mode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT) );
    
    GetNumberOfConsoleInputEvents(h, &num);
    SetConsoleMode( h, mode );

    return num;
}
#else
int my_kbhit() {
    static const int STDIN = 0;
    static int initialized = 0;

    if (! initialized) {
        // Use termios to turn off line buffering
        struct termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        term.c_lflag &= ~ECHO;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = 1;
    }

    int bytesWaiting;
    ioctl(STDIN, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}
#endif

static char Window_headlessBuffer[256];

void Window_UpdateHeadless()
{
    char buffer[32];
    size_t bytes_read = 0;

    if (my_kbhit() > 0) {
#if defined(WIN64_MINGW) || (_WIN32)
        buffer[0] = my_getch();
        buffer[1] = 0;
        bytes_read = 1;
#else
        int fd = STDIN_FILENO;
        bytes_read = read(fd, buffer, sizeof(buffer)-1);
        buffer[bytes_read] = 0;
#endif

        for (int i = 0; i < bytes_read; i++)
        {
            if (buffer[i] == '\n' || buffer[i] == '\r') {
                printf("\r> %s\n", Window_headlessBuffer);
                sithConsole_ExeCommand(Window_headlessBuffer);
                memset(Window_headlessBuffer, 0, sizeof(Window_headlessBuffer));
                continue;
            }
            else if (buffer[i] == 0x7F && strlen(Window_headlessBuffer)) {
                Window_headlessBuffer[strlen(Window_headlessBuffer)-1] = 0;
                printf("\r> %s ", Window_headlessBuffer);
                continue;
            }
            else if (buffer[i] < ' ' || buffer[i] > '~')
            {
                continue;
            }

            char tmp[2] = {buffer[i], 0};
            strncat(Window_headlessBuffer, tmp, 255);
        }
    }
    
    printf("\r> %s", Window_headlessBuffer);
    //printf("> %x %x %s\n", buffer[0], my_kbhit(), Window_headlessBuffer);
    fflush(stdout);

    if (Window_resized)
    {
        jkMain_FixRes();
        if (!jkGui_SetModeMenu(0))
        {
            stdDisplay_SetMode(0, 0, 0);
            //jkMain_FixRes();
        }

        jkGui_SetModeGame();
        
        Window_resized = 0;
    }
    
    int sampleTime_roundtrip = SDL_GetTicks() - Window_lastSampleTime;
    //printf("%u\n", sampleTime_roundtrip);
    Window_lastSampleTime = SDL_GetTicks();

    static int sampleTime_delay = 0;
    int menu_framelimit_amt_ms = 6;

    if (!jkGame_isDDraw)
    {

        if (!jkGuiBuildMulti_bRendering) {
            std3D_StartScene();
#ifdef QUAKE_CONSOLE
            jkQuakeConsole_Render();
#endif
            std3D_DrawMenu();
            std3D_EndScene();
            //SDL_GL_SwapWindow(displayWindow);
        }
        else {
#ifdef QUAKE_CONSOLE
            jkQuakeConsole_Render();
#endif
            std3D_DrawMenu();
            //SDL_GL_SwapWindow(displayWindow);
            //menu_framelimit_amt_ms = 64;
        }
    }
    else
    {
        // Save mouse position for menu
        if (jkGame_isDDraw != last_jkGame_isDDraw) {
            Window_menu_mouseX = Window_mouseX;
            Window_menu_mouseY = Window_mouseY;
            Window_lastXRel = 0;
            Window_lastYRel = 0;
        }
    }

    // Keep entire loop at 6ms (150FPS)
    if (sampleTime_roundtrip < menu_framelimit_amt_ms) {
        sampleTime_delay++;
    }
    else {
        sampleTime_delay--;
    }
    if (sampleTime_delay <= 0) {
        sampleTime_delay = 1;
    }
    if (sampleTime_delay >= menu_framelimit_amt_ms) {
        sampleTime_delay = menu_framelimit_amt_ms;
    }
    SDL_Delay(sampleTime_delay);

    last_jkGame_isDDraw = jkGame_isDDraw;
    last_jkQuakeConsole_bOpen = jkQuakeConsole_bOpen;
}

void Window_SdlUpdate()
{
    if (Main_bHeadless)
    {
        Window_UpdateHeadless();
        return;
    }

    uint16_t left, right;
    uint32_t pos, msgl, msgr;
    int hasLeft, hasRight;
    SDL_Event event;
    SDL_MouseButtonEvent* mevent;

    // HACK: Escape key for controllers
    extern int stdControl_bControllerEscapeKey;
    extern int stdControl_bControllerEscapeKey_last;

    while (SDL_PollEvent(&event))
    {
        int bIsOdin = 0;
        int bIsGamepad = 0;

        if (event.type == SDL_EVENT_JOYSTICK_BUTTON_DOWN || event.type == SDL_EVENT_JOYSTICK_BUTTON_UP) {
            const char* name = SDL_GetJoystickNameForID(event.jbutton.which);
            bIsOdin = name && strcmp(name, "Odin Controller") == 0;
            bIsGamepad = SDL_IsGamepad(event.jbutton.which);
        }
        if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN || event.type == SDL_EVENT_GAMEPAD_BUTTON_UP) {
            bIsGamepad = 1;
        }

        if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST)
        {
            Window_HandleWindowEvent(&event);
            continue;
        }

        switch (event.type)
        {
            case SDL_EVENT_JOYSTICK_ADDED: {
                stdControl_bReadJoysticks = 1;
                stdControl_InitSdlJoysticks();
                break;
            }
            case SDL_EVENT_JOYSTICK_REMOVED: {
                stdControl_InitSdlJoysticks();
                break;
            }

            case SDL_EVENT_TEXT_INPUT:
                for (int i = 0; i < _strlen(event.text.text); i++)
                {
                    Window_msg_main_handler(g_hWnd, WM_CHAR, event.text.text[i], 0);
                }
                break;
            case SDL_EVENT_KEY_DOWN:
                //stdPlatform_Printf("scancode %d\n", event.key.scancode);
                //handleKey(&event.key.keysym, WM_KEYDOWN, 0x1);
                if (event.key.key == SDLK_ESCAPE)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_ESCAPE, event.key.repeat & 0xFFFF);
                    Window_msg_main_handler(g_hWnd, WM_CHAR, VK_ESCAPE, event.key.repeat & 0xFFFF);
                }
                else if (event.key.key == SDLK_PAGEUP)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_PRIOR, event.key.repeat & 0xFFFF);
                }
                else if (event.key.key == SDLK_PAGEDOWN)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_NEXT, event.key.repeat & 0xFFFF);
                }
                else if (event.key.key == SDLK_LEFT)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_LEFT, event.key.repeat & 0xFFFF);
                }
                else if (event.key.key == SDLK_RIGHT)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_RIGHT, event.key.repeat & 0xFFFF);
                }
                else if (event.key.key == SDLK_UP)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_UP, event.key.repeat & 0xFFFF);
                }
                else if (event.key.key == SDLK_DOWN)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_DOWN, event.key.repeat & 0xFFFF);
                }
                else if (event.key.key == SDLK_BACKSPACE)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_BACK, event.key.repeat & 0xFFFF);
                    Window_msg_main_handler(g_hWnd, WM_CHAR, VK_BACK, event.key.repeat & 0xFFFF);
                }
                else if (event.key.key == SDLK_DELETE)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_DELETE, event.key.repeat & 0xFFFF);
                    //Window_msg_main_handler(g_hWnd, WM_CHAR, VK_DELETE, 0);
                }
                else if (event.key.key == SDLK_INSERT)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_INSERT, event.key.repeat & 0xFFFF);
                    Window_msg_main_handler(g_hWnd, WM_CHAR, VK_INSERT, 0);
                }
                else if (event.key.key == SDLK_RETURN)
                {
                    // HACK apparently Windows buffers these events in some way, but to replicate the behavior in jkGUI we just spam KEYFIRST
                    Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_RETURN, event.key.repeat & 0xFFFF);
                    Window_msg_main_handler(g_hWnd, WM_CHAR, VK_RETURN, event.key.repeat & 0xFFFF);
                }
                else if (event.key.key == SDLK_LSHIFT)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_LSHIFT, event.key.repeat & 0xFFFF);
                }
                else if (event.key.key == SDLK_RSHIFT)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_RSHIFT, event.key.repeat & 0xFFFF);
                }
                else if (event.key.key == SDLK_TAB)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_TAB, event.key.repeat & 0xFFFF);
                    Window_msg_main_handler(g_hWnd, WM_CHAR, VK_TAB, event.key.repeat & 0xFFFF);
                }
                else if (event.key.key == SDLK_END)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_END, event.key.repeat & 0xFFFF);
                    //Window_msg_main_handler(g_hWnd, WM_CHAR, 0x23, 0);
                }
                else if (event.key.key == SDLK_HOME)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_HOME, event.key.repeat & 0xFFFF);
                    //Window_msg_main_handler(g_hWnd, WM_CHAR, 0x24, 0);
                }
                else if (event.key.key == SDLK_GRAVE)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_OEM_3, event.key.repeat & 0xFFFF);
                }
                else if (event.key.scancode == SDL_SCANCODE_AC_BACK) {
                    Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_ESCAPE, 0);
                    Window_msg_main_handler(g_hWnd, WM_CHAR, VK_ESCAPE, event.key.repeat & 0xFFFF);
                }

                //if (!event.key.repeat)
                //    stdControl_SetSDLKeydown(event.key.scancode, 1, event.key.timestamp);
                break;
            case SDL_EVENT_KEY_UP:
                if (event.key.key == SDLK_ESCAPE)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYUP, VK_ESCAPE, 0);
                }
                else if (event.key.key == SDLK_PAGEUP)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYUP, VK_PRIOR, 0);
                }
                else if (event.key.key == SDLK_PAGEDOWN)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYUP, VK_NEXT, 0);
                }
                else if (event.key.key == SDLK_LEFT)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYUP, VK_LEFT, 0);
                }
                else if (event.key.key == SDLK_RIGHT)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYUP, VK_RIGHT, 0);
                }
                else if (event.key.key == SDLK_UP)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYUP, VK_UP, 0);
                }
                else if (event.key.key == SDLK_DOWN)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYUP, VK_DOWN, 0);
                }
                else if (event.key.key == SDLK_BACKSPACE)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYUP, VK_BACK, 0);
                }
                else if (event.key.key == SDLK_DELETE)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYUP, VK_DELETE, 0);
                }
                else if (event.key.key == SDLK_INSERT)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYUP, VK_INSERT, 0);
                }
                else if (event.key.key == SDLK_RETURN)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYUP, VK_RETURN, 0); // 0xB?
                }
                else if (event.key.key == SDLK_LSHIFT)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYUP, VK_LSHIFT, 0);
                }
                else if (event.key.key == SDLK_RSHIFT)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYUP, VK_RSHIFT, 0);
                }
                else if (event.key.key == SDLK_TAB)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYUP, VK_TAB, 0);
                }
                else if (event.key.key == SDLK_END)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYUP, VK_END, 0);
                }
                else if (event.key.key == SDLK_HOME)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYUP, VK_HOME, 0);
                }
                else if (event.key.key == SDLK_GRAVE)
                {
                    Window_msg_main_handler(g_hWnd, WM_KEYUP, VK_OEM_3, 0);
                }
                else if (event.key.scancode == SDL_SCANCODE_AC_BACK) {
                    Window_msg_main_handler(g_hWnd, WM_KEYUP, VK_ESCAPE, 0);
                }
                //handleKey(&event.key.keysym, WM_KEYUP, 0xc0000001);

                if (jkQuakeConsole_bOpen) break; // Hijack all input to console

                stdControl_SetSDLKeydown(event.key.scancode, 0, (uint32_t)(event.key.timestamp / 1000000));
                break;
            case SDL_EVENT_MOUSE_MOTION:
                if (getenv("OPENJKDF2_VALIDATE_MOUSE_LATENCY") &&
                    (event.motion.xrel != 0.0f || event.motion.yrel != 0.0f))
                {
                    char latencyEvent[192];
                    uint64_t nowNs = SDL_GetTicksNS();
                    uint64_t queueUs = nowNs >= event.motion.timestamp
                        ? (nowNs - event.motion.timestamp) / 1000ULL : 0;
                    Window_validationMouseEventNs = event.motion.timestamp;
                    Window_validationMouseHandlerNs = nowNs;
                    ++Window_validationMouseSequence;
                    snprintf(latencyEvent, sizeof(latencyEvent),
                             "mouse_latency stage=dispatch seq=%u queue_us=%llu",
                             Window_validationMouseSequence,
                             (unsigned long long)queueUs);
                    diag_log_event(DIAG_SEVERITY_INFO, "input", latencyEvent);
                }
                Window_HandleMouseMove(&event.motion);
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:

                mevent = (SDL_MouseButtonEvent*)&event;
                left = 0;
                right = 0;
                hasLeft = 0;
                hasRight = 0;
                if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                {
                    left = (mevent->button == SDL_BUTTON_LEFT ? 1 : 0);
                    right = (mevent->button == SDL_BUTTON_RIGHT ? 2 : 0);
                    
                    if (left)
                        hasLeft = 1;
                    if (right)
                        hasRight = 1;
                }
                else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
                {
                    left = (mevent->button == SDL_BUTTON_LEFT ? 0 : 1);
                    right = (mevent->button == SDL_BUTTON_RIGHT ? 0 : 2);
                    
                    if (!left)
                        hasLeft = 1;
                    if (!right)
                        hasRight = 1;
                }
                
                if (hasLeft)
                    Window_bMouseLeft = left;
                if (hasRight)
                    Window_bMouseRight = right;

                Window_mouseX = (int)mevent->x;
                Window_mouseY = (int)mevent->y;// - (Window_ySize - 480);

                pos = ((Window_mouseX) & 0xFFFF) | (((Window_mouseY) << 16) & 0xFFFF0000);
                msgl = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ? WM_LBUTTONDOWN : WM_LBUTTONUP);
                msgr = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ? WM_RBUTTONDOWN : WM_RBUTTONUP);

                if (jkQuakeConsole_bOpen) break; // Hijack all input to console
                
                if (hasLeft)
                    Window_msg_main_handler(g_hWnd, msgl, left | right, pos);
                if (hasRight)
                    Window_msg_main_handler(g_hWnd, msgr, left | right, pos);

                //stdControl_UpdateKeyState(KEY_MOUSE_B1, Window_bMouseLeft, mevent->timestamp);
                //stdControl_UpdateKeyState(KEY_MOUSE_B2, Window_bMouseRight, mevent->timestamp);

                break;
            case SDL_EVENT_MOUSE_WHEEL:
                Window_mouseWheelY = (int)event.wheel.y;
                Window_mouseWheelX = (int)event.wheel.x;

                if (jkQuakeConsole_bOpen) break; // Hijack all input to console
                break;

            // HACK: Escape key for controllers
            case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
            case SDL_EVENT_JOYSTICK_BUTTON_UP:
                if (!bIsGamepad) {
                    //stdPlatform_Printf("button %d, %d\n", event.jbutton.button, event.jbutton.state);
                }
                if (bIsOdin && !bIsGamepad && (event.jbutton.button == 6 || event.jbutton.button == 4)) {
                    stdControl_bControllerEscapeKey = event.jbutton.down;
                }
                else if (!bIsGamepad && jkCutscene_isRendering && event.type == SDL_EVENT_JOYSTICK_BUTTON_DOWN && event.jbutton.button == 3) { // y
                    Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_SPACE, 0);
                    Window_msg_main_handler(g_hWnd, WM_CHAR, VK_SPACE, 0);
                }
                else if (!bIsGamepad && jkCutscene_isRendering  && event.type == SDL_EVENT_JOYSTICK_BUTTON_DOWN&& event.jbutton.button == 2) { // x
                    Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_SPACE, 0);
                    Window_msg_main_handler(g_hWnd, WM_CHAR, VK_SPACE, 0);
                }
                else if (!bIsGamepad && jkCutscene_isRendering && event.type == SDL_EVENT_JOYSTICK_BUTTON_DOWN && event.jbutton.button == 1) { // b
                    Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_ESCAPE, 0);
                    Window_msg_main_handler(g_hWnd, WM_CHAR, VK_ESCAPE, 0);
                }
                else if (!bIsGamepad && jkCutscene_isRendering && event.type == SDL_EVENT_JOYSTICK_BUTTON_DOWN && event.jbutton.button == 0) { // a
                    Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_ESCAPE, 0);
                    Window_msg_main_handler(g_hWnd, WM_CHAR, VK_ESCAPE, 0);
                }
                break;

            case SDL_EVENT_JOYSTICK_AXIS_MOTION:
                if (event.jaxis.which == 0) {
                    //stdPlatform_Printf("axis %d, %d\n", event.jaxis.axis, event.jaxis.value);
                }
                break;

            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            case SDL_EVENT_GAMEPAD_BUTTON_UP:
                if (bIsGamepad) {
                    //stdPlatform_Printf("gpad button %d, %d\n", event.gbutton.button, event.gbutton.state);
                    if (event.gbutton.button == SDL_GAMEPAD_BUTTON_START || event.gbutton.button == SDL_GAMEPAD_BUTTON_BACK) {
                        stdControl_bControllerEscapeKey = event.gbutton.down;
                    }
                    else if (jkCutscene_isRendering && event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN && event.gbutton.button == SDL_GAMEPAD_BUTTON_NORTH) { // y
                        Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_SPACE, 0);
                        Window_msg_main_handler(g_hWnd, WM_CHAR, VK_SPACE, 0);
                    }
                    else if (jkCutscene_isRendering  && event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN && event.gbutton.button == SDL_GAMEPAD_BUTTON_WEST) { // x
                        Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_SPACE, 0);
                        Window_msg_main_handler(g_hWnd, WM_CHAR, VK_SPACE, 0);
                    }
                    else if (jkCutscene_isRendering && event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN && event.gbutton.button == SDL_GAMEPAD_BUTTON_EAST) { // b
                        Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_ESCAPE, 0);
                        Window_msg_main_handler(g_hWnd, WM_CHAR, VK_ESCAPE, 0);
                    }
                    else if (jkCutscene_isRendering && event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN && event.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH) { // a
                        Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_ESCAPE, 0);
                        Window_msg_main_handler(g_hWnd, WM_CHAR, VK_ESCAPE, 0);
                    }
                }
                break;
            case SDL_EVENT_GAMEPAD_AXIS_MOTION:
                //stdPlatform_Printf("Controller %d Axis %d moved to %d\n", 
                //       event.caxis.which, event.caxis.axis, event.caxis.value);
                break;

            case SDL_EVENT_QUIT:
                stdPlatform_Printf("Quit!\n");
                Window_SetGameplayMouseCapture(0);

                // Added
                if (jkPlayer_bHasLoadedSettingsOnce) {
                    jkPlayer_WriteConf(jkPlayer_playerShortName);
                }
                
                exit(-1);
                break;
            default:
                break;
        }
    }

    // HACK: Escape key for controllers
    if (stdControl_bControllerEscapeKey && !stdControl_bControllerEscapeKey_last) {
        Window_msg_main_handler(g_hWnd, WM_KEYFIRST, VK_ESCAPE, 0);
        Window_msg_main_handler(g_hWnd, WM_CHAR, VK_ESCAPE, 0);
    }
    stdControl_bControllerEscapeKey_last = stdControl_bControllerEscapeKey;
    
    if (Window_resized)
    {
        jkMain_FixRes();
        if (!jkGui_SetModeMenu(0))
        {
            stdDisplay_SetMode(0, 0, 0);
            //jkMain_FixRes();
        }
        
        Window_resized = 0;
    }
    
    static int sampleTime_delay = 0;
    int sampleTime_roundtrip = SDL_GetTicks() - Window_lastSampleTime;
    //printf("%u\n", sampleTime_roundtrip);
    Window_lastSampleTime = SDL_GetTicks();

    static int jkPlayer_enableVsync_last = 999;
    int menu_framelimit_amt_ms = 16;

    if (jkPlayer_enableVsync_last != jkPlayer_enableVsync)
    {
        Window_ApplyVsyncMode(jkPlayer_enableVsync);
    }

    if (!jkGame_isDDraw)
    {
        // Restore menu mouse position
        if (jkGame_isDDraw != last_jkGame_isDDraw) {
            SDL_WarpMouseInWindow(displayWindow, Window_menu_mouseX, Window_menu_mouseY);
        }

        Window_SetGameplayMouseCapture(0);

        if (!jkGuiBuildMulti_bRendering) {
            std3D_StartScene();
#ifdef QUAKE_CONSOLE
            jkQuakeConsole_Render();
#endif
            std3D_DrawMenu();
            std3D_EndScene();
            SDL_GL_SwapWindow(displayWindow);
        }
        else {
#ifdef QUAKE_CONSOLE
            jkQuakeConsole_Render();
#endif
            std3D_DrawMenu();
            SDL_GL_SwapWindow(displayWindow);
            //menu_framelimit_amt_ms = 64;
        }

        if (Window_needsRecreate) {
            std3D_PurgeEntireTextureCache();
            Window_RecreateSDL2Window();
        }
        
        // Keep menu FPS at 60FPS, to avoid cranking the GPU unnecessarily.
        if (sampleTime_roundtrip < menu_framelimit_amt_ms) {
            sampleTime_delay++;
        }
        else {
            sampleTime_delay--;
        }
        if (sampleTime_delay <= 0) {
            sampleTime_delay = 1;
        }
        if (sampleTime_delay >= menu_framelimit_amt_ms) {
            sampleTime_delay = menu_framelimit_amt_ms;
        }
        SDL_Delay(sampleTime_delay);
    }
    else
    {
        // Save mouse position for menu
        if (jkGame_isDDraw != last_jkGame_isDDraw) {
            Window_menu_mouseX = Window_mouseX;
            Window_menu_mouseY = Window_mouseY;
            Window_lastXRel = 0;
            Window_lastYRel = 0;
        }

#ifdef QUAKE_CONSOLE

        if (jkQuakeConsole_bOpen && jkQuakeConsole_bOpen != last_jkQuakeConsole_bOpen) {
            SDL_WarpMouseInWindow(displayWindow, Window_menu_mouseX, Window_menu_mouseY);
        }
        else if (!jkQuakeConsole_bOpen && jkQuakeConsole_bOpen != last_jkQuakeConsole_bOpen) {
            Window_menu_mouseX = Window_mouseX;
            Window_menu_mouseY = Window_mouseY;
            Window_lastXRel = 0;
            Window_lastYRel = 0;
        }

        if (jkQuakeConsole_bOpen)
        {
            Window_SetGameplayMouseCapture(0);
        }

        if (!jkQuakeConsole_bOpen && SDL_GetWindowFlags(displayWindow) & SDL_WINDOW_MOUSE_FOCUS) {
            Window_SetGameplayMouseCapture(1);
            //SDL_WarpMouseInWindow(displayWindow, 100, 100);
        }
        else
        {
            Window_SetGameplayMouseCapture(0);
        }
#endif
    }

    jkPlayer_enableVsync_last = jkPlayer_enableVsync;

    last_jkGame_isDDraw = jkGame_isDDraw;
#ifdef QUAKE_CONSOLE
    last_jkQuakeConsole_bOpen = jkQuakeConsole_bOpen;
#endif
}

void Window_SdlVblank()
{
    if (Main_bHeadless) return;

    if (!Window_bSwapLogged)
    {
        char swapEvent[192];
        snprintf(swapEvent, sizeof(swapEvent),
                 "swap_started mode=%s width=%d height=%d vsync=%s",
                 display_mode_name(Window_displayMode), Window_screenXSize,
                 Window_screenYSize, PresentationMode_VsyncName(Window_appliedVsync));
        diag_log_event(DIAG_SEVERITY_INFO, "presentation", swapEvent);
        Window_bSwapLogged = 1;
    }

    //static uint32_t roundtrip = 0;
    //uint32_t before = stdPlatform_GetTimeMsec();
#ifdef ARCH_WASM
    if (!jkGuiBuildMulti_bRendering)
#endif
    SDL_GL_SwapWindow(displayWindow);
    //uint32_t after = stdPlatform_GetTimeMsec();
    //printf("%u %u\n", after-before, before-roundtrip);

    //roundtrip = before;

    if (Window_needsRecreate)
        Window_RecreateSDL2Window();

#ifdef ARCH_WASM
    //emscripten_sleep(1);
#endif
}

#ifdef ARCH_WASM
// Fixed: this is embedded JavaScript, not C -- `canvas` refers to the
// <canvas id="canvas"> element the browser auto-exposes as a global (per the
// HTML "named access on the Window object" spec), not the C rdCanvas struct.
// An automated rd*-struct-member rename pass (d84fc7328) mistakenly renamed
// it to `pCanvas` along with the real C-side renames, breaking WASM at
// runtime with "pCanvas is not defined" (SDL_CreateWindow's size args).
EM_JS(int, canvas_get_width, (), {
  return canvas.width;
});

EM_JS(int, canvas_get_height, (), {
  return canvas.height;
});
#endif

void Window_RecreateSDL2Window()
{
#ifdef ARCH_WASM
    static int onlyOnce = 0;
    if (onlyOnce) {
        return;
    }
    onlyOnce = 1;
#endif

    if (Main_bHeadless) return;

    stdPlatform_Printf("Recreating SDL2 Window!\n");
    Window_needsRecreate = 0;

    if (displayWindow) {
        Window_SetGameplayMouseCapture(0);
        std3D_FreeResources();
        SDL_GL_DestroyContext(glWindowContext);
        SDL_DestroyWindow(displayWindow);
        glWindowContext = NULL;
        displayWindow = NULL;
    }

    // HACK: side-step the json stuff
    if (Window_bShouldPopSteamKeyboard) {
        Window_isFullscreen = 1;
        Window_isHiDpi = 1;
    }

    SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

#ifdef WIN64_STANDALONE
    // SDL_HINT_WINDOWS_DPI_AWARENESS has no SDL3 equivalent (removed) -- SDL3
    // windows are DPI-aware by default.
#endif

    if (Window_isHiDpi)
        flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
    else
        flags &= ~SDL_WINDOW_HIGH_PIXEL_DENSITY;

    SDL_Rect desktop_bounds = { 0, 0, Window_screenXSize, Window_screenYSize };
    if (Window_displayMode == DISPLAY_MODE_BORDERLESS) {
        SDL_DisplayID display = Window_DisplayIdForOrdinal(Window_displayMonitor);
        if (display && SDL_GetDisplayBounds(display, &desktop_bounds)) {
            Window_screenXSize = desktop_bounds.w;
            Window_screenYSize = desktop_bounds.h;
            Window_xPos = desktop_bounds.x;
            Window_yPos = desktop_bounds.y;
        }
        flags |= SDL_WINDOW_BORDERLESS;
        flags &= ~SDL_WINDOW_RESIZABLE;
    }

    else if (Window_displayMode == DISPLAY_MODE_WINDOWED) {
        SDL_DisplayID display = Window_DisplayIdForOrdinal(Window_displayMonitor);
        SDL_Rect bounds;
        Window_screenXSize = Window_windowWidth;
        Window_screenYSize = Window_windowHeight;
        if (display && SDL_GetDisplayBounds(display, &bounds)) {
            Window_xPos = bounds.x + (bounds.w - Window_screenXSize) / 2;
            Window_yPos = bounds.y + (bounds.h - Window_screenYSize) / 2;
        }
    }

#if defined(ARCH_WASM)
    //flags &= ~SDL_WINDOW_RESIZABLE;
#endif

#ifdef TARGET_ANDROID
    // Fixed: SDL_WINDOW_SHOWN removed (windows are shown by default in SDL3), but this
    // also dropped SDL_WINDOW_OPENGL, which SDL3 requires on the window before
    // SDL_GL_CreateContext will succeed (SDL2 was more lenient on Android) -- caused
    // "Failed to initialize SDL OpenGL Context // The specified window isn't an OpenGL
    // window" at runtime.
    flags = SDL_WINDOW_OPENGL;
#endif

    // SDL3 SDL_CreateWindow() dropped the x/y position params; position is set
    // separately below via SDL_SetWindowPosition() on desktop.
#ifdef ARCH_WASM
    displayWindow = SDL_CreateWindow(Window_isHiDpi ? "OpenJKDF2 HiDPI" : "OpenJKDF2", canvas_get_width(), canvas_get_height(), flags);
#elif defined(TARGET_ANDROID)
    displayWindow = SDL_CreateWindow(Window_isHiDpi ? "OpenJKDF2 HiDPI" : "OpenJKDF2", Window_screenXSize, Window_screenYSize, flags);
#else
    displayWindow = SDL_CreateWindow(Window_isHiDpi ? "OpenJKDF2 HiDPI" : "OpenJKDF2", Window_screenXSize, Window_screenYSize, flags);
#endif
    if (!displayWindow) {
        char errtmp[256];
        snprintf(errtmp, 256, "!! Failed to create SDL2 window !!\n%s", SDL_GetError());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", errtmp, NULL);
        exit (-1);
    }
    //SDL_SetRenderDrawBlendMode(displayRenderer, SDL_BLENDMODE_BLEND);

#if !defined(ARCH_WASM) && !defined(TARGET_ANDROID)
    SDL_SetWindowPosition(displayWindow, Window_xPos, Window_yPos);
#endif

    if (Window_displayMode == DISPLAY_MODE_EXCLUSIVE && Window_bRestorationGuardReady) {
        SDL_SetWindowFullscreen(displayWindow, true);
    }
    SDL_RaiseWindow(displayWindow);

    Window_contextFallbackTier = 0;
    glWindowContext = SDL_GL_CreateContext(displayWindow);
    
    // Retry with 3.30 instead
    if (glWindowContext == NULL)
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
        glWindowContext = SDL_GL_CreateContext(displayWindow);
        if (glWindowContext)
            Window_contextFallbackTier = 1;
    }

    // Retry with 3.20 and this thing instead
    if (glWindowContext == NULL)
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
        glWindowContext = SDL_GL_CreateContext(displayWindow);
        if (glWindowContext)
            Window_contextFallbackTier = 2;
    }
    
    if (glWindowContext == NULL)
    {
        char errtmp[256];
        snprintf(errtmp, 256, "!! Failed to initialize SDL OpenGL context !!\n%s", SDL_GetError());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", errtmp, NULL);
        exit(-1);
    }

    SDL_GL_MakeCurrent(displayWindow, glWindowContext);
    Window_CopyRendererString(Window_rendererVendor, sizeof(Window_rendererVendor), GL_VENDOR);
    Window_CopyRendererString(Window_rendererGpu, sizeof(Window_rendererGpu), GL_RENDERER);
    Window_CopyRendererString(Window_rendererDriver, sizeof(Window_rendererDriver), GL_VERSION);
    {
        char glVersion[128];
        char glslVersion[128];
        Window_CopyRendererString(glVersion, sizeof(glVersion), GL_VERSION);
        Window_CopyRendererString(glslVersion, sizeof(glslVersion), GL_SHADING_LANGUAGE_VERSION);
        snprintf(Window_rendererApi, sizeof(Window_rendererApi), "OpenGL %s / GLSL %s", glVersion, glslVersion);
    }
    Window_ApplyVsyncMode(jkPlayer_enableVsync);
#ifndef TARGET_ANDROID
    SDL_StartTextInput(displayWindow);
#endif

    SDL_GetWindowSizeInPixels(displayWindow, &Window_xSize, &Window_ySize);
    SDL_GetWindowSize(displayWindow, &Window_screenXSize, &Window_screenYSize);

    Window_resized = 1;
}

void Window_Main_Loop()
{
    static uint64_t frameDeadlineNs = 0;
    static int previousConfiguredRate = FRAME_RATE_UNLIMITED;
    int desktopRefreshRate = 0;
    int targetRate;
    uint64_t periodNs;
    uint64_t nowNs;
    const SDL_DisplayMode* desktopMode;

    jkMain_GuiAdvance(); // TODO needed?
#ifdef TARGET_DREAMCAST
    // Loop-phase tracer (see std3D border tracer). BLUE (set in std3D_EndScene) still
    // showing here => hang inside the tick after the render; CYAN => hang in the
    // present; WHITE => hang at the top of the next tick's sim (before its render).
    std3D_BorderTrace(0, 255, 255);   // CYAN: tick returned, about to present
#endif
    Window_msg_main_handler(g_hWnd, WM_PAINT, 0, 0);
#ifdef TARGET_DREAMCAST
    std3D_BorderTrace(255, 255, 255); // WHITE: presented, looping to next tick
#endif

    if (jkPlayer_fpslimit == FRAME_RATE_DESKTOP_REFRESH)
    {
        desktopMode = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());
        if (desktopMode)
            desktopRefreshRate = (int)(desktopMode->refresh_rate + 0.5f);
    }
    targetRate = FrameRate_ResolveTarget(jkPlayer_fpslimit, desktopRefreshRate);
    periodNs = FrameRate_PeriodNanoseconds(targetRate);
    nowNs = SDL_GetTicksNS();
    FrameTelemetry_Record(nowNs);

    if (jkPlayer_fpslimit != previousConfiguredRate)
        frameDeadlineNs = 0;
    previousConfiguredRate = jkPlayer_fpslimit;

    frameDeadlineNs = FrameRate_NextDeadline(frameDeadlineNs, nowNs, periodNs, NULL);
    if (frameDeadlineNs > nowNs)
        SDL_DelayPrecise(frameDeadlineNs - nowNs);

    //Window_SdlUpdate();
}

int Window_Main_Linux(int argc, char** argv)
{
    char cmdLine[1024];
    int result;

    // Init SDL
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    SDL_SetHint(SDL_HINT_APP_NAME, "OpenJKDF2");

#if defined(TARGET_ANDROID)
    //SDL_SetHint(SDL_HINT_JOYSTICK_DEBUG, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_JOY_CONS, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS4, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_XBOX, "1");
    //SDL_SetHint(SDL_HINT_AUTO_UPDATE_JOYSTICKS, "1");
    // SDL_HINT_ACCELEROMETER_AS_JOYSTICK has no SDL3 equivalent (removed).
    SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
    SDL_SetHint("SDL_MIXER_DEBUG_MUSIC_INTERFACES", "1");
    SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "aaudio"); // This is fine for music tbh
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
#endif

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD);

    
    if ((SDL_GetHintBoolean("SteamClientLaunch", 0) || SDL_GetHintBoolean("SteamOS", 0) || SDL_GetHintBoolean("SteamDeck", 0)) && SDL_GetHintBoolean("SteamGamepadUI", 0)) {
        Window_bShouldPopSteamKeyboard = 1;
        Window_isFullscreen = 1;
        Window_isHiDpi = 1;
    }

#if defined(RENDER_GL11)
    // Legacy fixed-function GL 1.1 backend: request a compatibility context.
    // Requesting 1.1 makes desktop drivers return their highest legacy
    // (compatibility) context, which is what old XP-era hardware exposes.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
#elif defined(MACOS)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
#else

#if defined(WIN64_STANDALONE)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

    // apitrace
#if 0
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
#endif
#elif defined(TARGET_ANDROID)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
#elif defined(ARCH_WASM)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
#endif

#endif

    Window_RecreateSDL2Window();
#if !defined(TARGET_ANDROID) && !defined(ARCH_WASM) && !defined(RENDER_GL11)
    glewInit();
#endif
    
    //SDL_RenderClear(displayRenderer);
    //SDL_RenderPresent(displayRenderer);
    
    
    strcpy(cmdLine, "");
    
    g_handler_count = 0;
    g_thing_two_some_dialog_count = 0;
    g_should_exit = 0;
    g_window_not_destroyed = 0;
    g_hInstance = 0;//hInstance;
    g_nShowCmd = 0;//nShowCmd;
    
    if (!storage_paths_build_legacy_command(argc, (const char* const*)argv, cmdLine, sizeof(cmdLine)))
        return 0;
    
    result = Main_Startup(cmdLine);

    int fullscreen = wuRegistry_GetBool("Window_isFullscreen", 1);
    int display_mode = wuRegistry_GetInt("Window_displayMode", fullscreen ? DISPLAY_MODE_BORDERLESS : DISPLAY_MODE_WINDOWED);
    int hidpi = wuRegistry_GetBool("Window_isHiDpi", 0);
    DisplaySettings saved_settings = {
        Window_bSafeMode ? DISPLAY_MODE_WINDOWED : display_mode_from_config(display_mode),
        wuRegistry_GetInt("Window_displayMonitor", 0),
        wuRegistry_GetInt("Window_windowWidth", WINDOW_DEFAULT_WIDTH),
        wuRegistry_GetInt("Window_windowHeight", WINDOW_DEFAULT_HEIGHT),
        wuRegistry_GetInt("Window_refreshHz", 0),
        hidpi
    };
    DisplaySelectionReason selection_reason;
    if (!Window_ApplyDisplaySettings(saved_settings, &selection_reason))
    {
        DisplaySettings safe_settings = {
            DISPLAY_MODE_WINDOWED, 0, WINDOW_DEFAULT_WIDTH, WINDOW_DEFAULT_HEIGHT, 0, 0
        };
        Window_ApplyDisplaySettings(safe_settings, &selection_reason);
        diag_log_event(DIAG_SEVERITY_WARNING, "display",
                       "saved_display_settings_invalid fallback=safe_windowed");
    }
    Window_RecreateSDL2Window();

    if (!result) return result;

    if (Main_bHeadless)
    {
        if (displayWindow) {
            Window_SetGameplayMouseCapture(0);
            std3D_FreeResources();
            SDL_GL_DestroyContext(glWindowContext);
            SDL_DestroyWindow(displayWindow);
        }
    }

    g_window_not_destroyed = 1;
    
    Window_msg_main_handler(g_hWnd, 0x1, 0, 0); // WM_CREATE
    Window_msg_main_handler(g_hWnd, 0x6, 2, 0); // WM_ACTIVATE
    Window_msg_main_handler(g_hWnd, 0x1C, 1, 0); // WM_ACTIVATEAPP
    Window_msg_main_handler(g_hWnd, 0x18, 0, 0); // WM_SHOWWINDOW
    Window_msg_main_handler(g_hWnd, WM_PAINT, 0, 0);


#ifdef ARCH_WASM
    //int fps = 0; // Use browser's requestAnimationFrame
    //emscripten_set_main_loop_arg(Window_Main_Loop, NULL, fps, 1);
    while (1)
    {
        Window_Main_Loop();
        if (g_should_exit) break;
    }
#else
    while (1)
    {
        Window_Main_Loop();
        if (g_should_exit) break;
    }
#endif

    // Added
    if (jkPlayer_bHasLoadedSettingsOnce) {
        jkPlayer_WriteConf(jkPlayer_playerShortName);
    }

    Main_Shutdown();
    return 1;
}

int Window_Main(HINSTANCE hInstance, int a2, char *lpCmdLine, int nShowCmd, LPCSTR lpWindowName)
{
    int result;

    g_handler_count = 0;
    g_thing_two_some_dialog_count = 0;
    g_should_exit = 0;
    g_window_not_destroyed = 0;
    g_hInstance = hInstance;
    g_nShowCmd = nShowCmd;
#if 0
    if (jk_RegisterClassExA(&wndClass))
    {
        if ( jk_FindWindowA("wKernel", lpWindowName) )
            jk_exit(-1);

        uint32_t hres = jk_GetSystemMetrics(1);
        uint32_t vres = jk_GetSystemMetrics(0);
        g_hWnd = jk_CreateWindowExA(0x40000u, "wKernel", lpWindowName, 0x90000000, 0, 0, vres, hres, 0, 0, hInstance, 0);

        if (g_hWnd)
        {
            g_hInstance = hInstance;
            jk_ShowWindow(g_hWnd, 1);
            jk_UpdateWindow(g_hWnd);
        }
    }

    stdGdi_SetHwnd(g_hWnd);
    stdGdi_SetHInstance(g_hInstance);
    jk_InitCommonControls();

    g_855E8C = 2 * jk_GetSystemMetrics(32);
    uint32_t metrics_32 = jk_GetSystemMetrics(32);
    g_855E90 = jk_GetSystemMetrics(15) + 2 * metrics_32;
    result = Main_Startup(lpCmdLine);

    if (!result) return result;

    
    g_window_not_destroyed = 1;

    while (1)
    {
        if (jk_PeekMessageA(&msg, 0, 0, 0, 0))
        {
            if (!jk_GetMessageA(&msg, 0, 0, 0))
            {
                result = msg.wParam;
                g_should_exit = 1;
                break;
            }

            uint32_t some_cnt = 0;
            if (g_thing_two_some_dialog_count > 0)
            {
#if 0
                v16 = &thing_three;
                do
                {
                    //TODO if ( jk_IsDialogMessageA(*v16, &msg) )
                    //  break;
                    ++some_cnt;
                    ++v16;
                }
                while ( some_cnt < g_thing_two_some_dialog_count );
#endif
            }

            if (some_cnt == g_thing_two_some_dialog_count)
            {
                jk_TranslateMessage(&msg);
                jk_DispatchMessageA(&msg);
            }

            if (!jk_PeekMessageA(&msg, 0, 0, 0, 0))
            {
                result = 0;
                if ( g_should_exit )
                    return result;
            }
        }

        //if (user32->stopping) break;

        jkMain_GuiAdvance();
    }
#endif
    result = 1;
    return result;
}

int Window_ShowCursorUnwindowed(int a1)
{
    return stdControl_ShowMouseCursor(a1);
}

int Window_DefaultHandler(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, void* unused)
{
    return 0;
}

int Window_MessageLoop()
{
    // Added: controller menuing
    jkGuiRend_UpdateController();

    jkMain_GuiAdvance();
    Window_msg_main_handler(g_hWnd, WM_PAINT, 0, 0);
    
    //Window_SdlUpdate();
    return 0;
}

#endif // SDL2_RENDER

void Window_SetDrawHandlers(WindowDrawHandler_t a1, WindowDrawHandler_t a2)
{
    Window_drawAndFlip = a1;
    Window_setCooperativeLevel = a2;
}

void Window_GetDrawHandlers(WindowDrawHandler_t *a1, WindowDrawHandler_t *a2)
{
    *a1 = Window_drawAndFlip;
    *a2 = Window_setCooperativeLevel;
}
