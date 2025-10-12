/**
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "pch.h"

#include <Windows.h>

#include <Commdlg.h>
#include <GL/wglew.h>
#include <shellapi.h>
#include <windowsx.h>

#include "framework/FileSystem.h"
#include "framework/Game.h"
#include "framework/Platform.h"
#include "math/Vector2.h"
#include "scripting/ScriptController.h"
#include "ui/Form.h"

#ifdef GP_USE_GAMEPAD
#include <XInput.h>

// Window defaults
constexpr auto DEFAULT_RESOLUTION_X = 1024;
constexpr auto DEFAULT_RESOLUTION_Y = 768;
constexpr auto DEFAULT_COLOR_BUFFER_SIZE = 32;
constexpr auto DEFAULT_DEPTH_BUFFER_SIZE = 24;
constexpr auto DEFAULT_STENCIL_BUFFER_SIZE = 8;

static double __timeTicksPerMillis;
static double __timeStart;
static double __timeAbsolute;
static bool __vsync = WINDOW_VSYNC;
static HINSTANCE __hinstance = 0;
static HWND __hwnd = 0;
static HDC __hdc = 0;
static HGLRC __hrc = 0;
static bool __mouseCaptured = false;
static POINT __mouseCapturePoint = { 0, 0 };
static bool __multiSampling = false;
static bool __cursorVisible = true;
static unsigned int __gamepadsConnected = 0;

static const unsigned int XINPUT_BUTTON_COUNT = 14;
static const unsigned int XINPUT_JOYSTICK_COUNT = 2;
static const unsigned int XINPUT_TRIGGER_COUNT = 2;

static XINPUT_STATE __xInputState;
static bool __connectedXInput[4];
#endif

namespace
{
constexpr int SHELL_EXECUTE_SUCCESS_THRESHOLD = 32;
constexpr int MOUSE_WHEEL_DELTA_DIVISOR = 120;
constexpr int OPENGL_MAJOR_VERSION = 3;
constexpr int OPENGL_MINOR_VERSION = 1;
constexpr DWORD MAX_PATH_LENGTH = 1024;

struct WindowCreationParams
{
    RECT rect;
    std::wstring windowName;
    bool fullscreen;
    bool resizable;
    int samples;
};

//----------------------------------------------------------------
/**
 * @brief Normalize a raw joystick X-axis integer into a floating-point value, applying a dead zone.
 * @param axisValue Raw integer joystick axis reading (typically in the range -32768 to 32767).
 * @param deadZone Dead-zone threshold; absolute axis values smaller than this are treated as zero.
 * @return A float in the range [-1.0, 1.0] representing the normalized axis value (0.0 if inside
 * the dead zone). Negative values indicate one direction, positive the other.
 */
float normalizeXInputJoystickAxis(int axisValue, int deadZone)
{
    const int absAxisValue = std::abs(axisValue);

    if (absAxisValue < deadZone)
    {
        return 0.0f;
    }

    // Determine the sign and max value
    const int sign = (axisValue < 0) ? -1 : 1;
    const int maxVal = (axisValue < 0) ? 32768 : 32767;

    return sign * (absAxisValue - deadZone) / static_cast<float>(maxVal - deadZone);
}

//----------------------------------------------------------------
/**
 * @brief Creates a native Windows window using the provided creation parameters, obtains its device
 * context, and centers the window on the screen.
 * @param params Pointer to a WindowCreationParams structure with optional settings (windowName,
 * rect, fullscreen, resizable). If null, default values are used.
 * @param hwnd Output pointer that receives the created window handle (HWND). On failure the value
 * may be nullptr.
 * @param hdc Output pointer that receives the window's device context (HDC). On failure the value
 * may be nullptr.
 * @return True if the window was created and its device context obtained successfully; false if
 * creation or device context acquisition failed.
 */
[[nodiscard]] bool createWindow(WindowCreationParams* params, HWND* hwnd, HDC* hdc)
{
    bool fullscreen = false;
    bool resizable = false;
    RECT rect = { CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT };
    std::wstring windowName;
    if (params)
    {
        windowName = params->windowName;
        memcpy(&rect, &params->rect, sizeof(RECT));
        fullscreen = params->fullscreen;
        resizable = params->resizable;
    }

    // Set the window style.
    DWORD style, styleEx;
    if (fullscreen)
    {
        style = WS_POPUP;
        styleEx = WS_EX_APPWINDOW;
    }
    else
    {
        if (resizable)
            style = WS_OVERLAPPEDWINDOW;
        else
            style = WS_POPUP | WS_BORDER | WS_CAPTION | WS_SYSMENU;
        styleEx = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    }
    style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

    // Adjust the window rectangle so the client size is the requested size.
    AdjustWindowRectEx(&rect, style, FALSE, styleEx);

    // Create the native Windows window.
    *hwnd = CreateWindowEx(styleEx,
                           L"gameplay",
                           windowName.c_str(),
                           style,
                           0,
                           0,
                           rect.right - rect.left,
                           rect.bottom - rect.top,
                           nullptr,
                           nullptr,
                           __hinstance,
                           nullptr);
    if (*hwnd == nullptr)
    {
        GP_ERROR("Failed to create window.");
        return false;
    }

    // Get the drawing context.
    *hdc = GetDC(*hwnd);
    if (*hdc == nullptr)
    {
        GP_ERROR("Failed to get device context.");
        return false;
    }

    // Center the window
    GetWindowRect(*hwnd, &rect);
    const int screenX = (GetSystemMetrics(SM_CXSCREEN) - rect.right) / 2;
    const int screenY = (GetSystemMetrics(SM_CYSCREEN) - rect.bottom) / 2;
    SetWindowPos(*hwnd, *hwnd, screenX, screenY, -1, -1, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    return true;
}

//----------------------------------------------------------------
/**
 * @brief Initializes OpenGL for the application by creating a temporary window/context to initialize GLEW, 
          selecting a suitable pixel format (optionally with multisampling), creating the final OpenGL context 
          (attempting a modern context via WGL extensions and falling back to the temporary context), enabling 
          vertical sync if available, and patching framebuffer functions when only EXT variants are present.
 * @param params Optional pointer to WindowCreationParams. If non-null, this function will use the provided parameters 
                (e.g., sample count) and create windows/contexts according to them. 
                If null, the function uses existing global window/DC handles (__hwnd, __hdc).
 * @return true if OpenGL was successfully initialized and a usable context/window was created or configured; 
           false on failure (errors are logged and temporary resources are cleaned up).
 */
[[nodiscard]] bool initializeGL(WindowCreationParams* params)
{
    // Create a temporary window and context to we can initialize GLEW and get access
    // to additional OpenGL extension functions. This is a neccessary evil since the
    // function for querying GL extensions is a GL extension itself.
    HWND hwnd = nullptr;
    HDC hdc = nullptr;

    if (params)
    {
        if (!createWindow(params, &hwnd, &hdc)) return false;
    }
    else
    {
        hwnd = __hwnd;
        hdc = __hdc;
    }

    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = DEFAULT_COLOR_BUFFER_SIZE;
    pfd.cDepthBits = DEFAULT_DEPTH_BUFFER_SIZE;
    pfd.cStencilBits = DEFAULT_STENCIL_BUFFER_SIZE;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixelFormat = ChoosePixelFormat(hdc, &pfd);
    if (pixelFormat == 0)
    {
        DestroyWindow(hwnd);
        GP_ERROR("Failed to choose a pixel format.");
        return false;
    }

    if (!SetPixelFormat(hdc, pixelFormat, &pfd))
    {
        DestroyWindow(hwnd);
        GP_ERROR("Failed to set the pixel format.");
        return false;
    }

    HGLRC tempContext = wglCreateContext(hdc);
    if (!tempContext)
    {
        DestroyWindow(hwnd);
        GP_ERROR("Failed to create temporary context for initialization.");
        return false;
    }
    wglMakeCurrent(hdc, tempContext);

    // Initialize GLEW
    if (GLEW_OK != glewInit())
    {
        wglDeleteContext(tempContext);
        DestroyWindow(hwnd);
        GP_ERROR("Failed to initialize GLEW.");
        return false;
    }

    if (wglChoosePixelFormatARB && wglCreateContextAttribsARB)
    {
        // Choose pixel format using wglChoosePixelFormatARB, which allows us to specify
        // additional attributes such as multisampling.
        //
        // Note: Keep multisampling attributes at the start of the attribute lists since code below
        // assumes they are array elements 0 through 3.
        int attribList[] = { WGL_SAMPLES_ARB,
                             params ? params->samples : 0,
                             WGL_SAMPLE_BUFFERS_ARB,
                             params ? (params->samples > 0 ? 1 : 0) : 0,
                             WGL_DRAW_TO_WINDOW_ARB,
                             GL_TRUE,
                             WGL_SUPPORT_OPENGL_ARB,
                             GL_TRUE,
                             WGL_DOUBLE_BUFFER_ARB,
                             GL_TRUE,
                             WGL_PIXEL_TYPE_ARB,
                             WGL_TYPE_RGBA_ARB,
                             WGL_COLOR_BITS_ARB,
                             DEFAULT_COLOR_BUFFER_SIZE,
                             WGL_DEPTH_BITS_ARB,
                             DEFAULT_DEPTH_BUFFER_SIZE,
                             WGL_STENCIL_BITS_ARB,
                             DEFAULT_STENCIL_BUFFER_SIZE,
                             0 };
        __multiSampling = params && params->samples > 0;

        UINT numFormats;
        if (!wglChoosePixelFormatARB(hdc, attribList, nullptr, 1, &pixelFormat, &numFormats)
            || numFormats == 0)
        {
            bool valid = false;
            if (params && params->samples > 0)
            {
                GP_WARN("Failed to choose pixel format with WGL_SAMPLES_ARB == %d. Attempting to "
                        "fallback to lower samples setting.",
                        params->samples);
                while (params->samples > 0)
                {
                    params->samples /= 2;
                    attribList[1] = params->samples;
                    attribList[3] = params->samples > 0 ? 1 : 0;
                    if (wglChoosePixelFormatARB(hdc, attribList, nullptr, 1, &pixelFormat, &numFormats)
                        && numFormats > 0)
                    {
                        valid = true;
                        GP_WARN("Found pixel format with WGL_SAMPLES_ARB == %d.", params->samples);
                        break;
                    }
                }

                __multiSampling = params->samples > 0;
            }

            if (!valid)
            {
                wglDeleteContext(tempContext);
                DestroyWindow(hwnd);
                GP_ERROR("Failed to choose a pixel format.");
                return false;
            }
        }

        // Create new/final window if needed
        if (params)
        {
            DestroyWindow(hwnd);
            hwnd = nullptr;
            hdc = nullptr;

            if (!createWindow(params, &__hwnd, &__hdc))
            {
                wglDeleteContext(tempContext);
                return false;
            }
        }

        // Set final pixel format for window
        if (!SetPixelFormat(__hdc, pixelFormat, &pfd))
        {
            GP_ERROR("Failed to set the pixel format: %d.", (int)GetLastError());
            return false;
        }

        // Create our new GL context
        int attribs[] = { WGL_CONTEXT_MAJOR_VERSION_ARB,
                          OPENGL_MAJOR_VERSION,
                          WGL_CONTEXT_MINOR_VERSION_ARB,
                          OPENGL_MINOR_VERSION,
                          0 };

        if (!(__hrc = wglCreateContextAttribsARB(__hdc, 0, attribs)))
        {
            wglDeleteContext(tempContext);
            GP_ERROR("Failed to create OpenGL context.");
            return false;
        }

        // Delete the old/temporary context and window
        wglDeleteContext(tempContext);

        // Make the new context current
        if (!wglMakeCurrent(__hdc, __hrc) || !__hrc)
        {
            GP_ERROR("Failed to make the window current.");
            return false;
        }
    }
    else // fallback to OpenGL 2.0 if wglChoosePixelFormatARB or wglCreateContextAttribsARB is nullptr.
    {
        // Context is already here, just use it.
        __hrc = tempContext;
        __hwnd = hwnd;
        __hdc = hdc;
    }

    // Vertical sync.
    if (wglSwapIntervalEXT)
        wglSwapIntervalEXT(__vsync ? 1 : 0);
    else
        __vsync = false;

    // Some old graphics cards support EXT_framebuffer_object instead of ARB_framebuffer_object.
    // Patch ARB_framebuffer_object functions to EXT_framebuffer_object ones since semantic is same.
    if (!GLEW_ARB_framebuffer_object && GLEW_EXT_framebuffer_object)
    {
        glBindFramebuffer = glBindFramebufferEXT;
        glBindRenderbuffer = glBindRenderbufferEXT;
        glBlitFramebuffer = glBlitFramebufferEXT;
        glCheckFramebufferStatus = glCheckFramebufferStatusEXT;
        glDeleteFramebuffers = glDeleteFramebuffersEXT;
        glDeleteRenderbuffers = glDeleteRenderbuffersEXT;
        glFramebufferRenderbuffer = glFramebufferRenderbufferEXT;
        glFramebufferTexture1D = glFramebufferTexture1DEXT;
        glFramebufferTexture2D = glFramebufferTexture2DEXT;
        glFramebufferTexture3D = glFramebufferTexture3DEXT;
        glFramebufferTextureLayer = glFramebufferTextureLayerEXT;
        glGenFramebuffers = glGenFramebuffersEXT;
        glGenRenderbuffers = glGenRenderbuffersEXT;
        glGenerateMipmap = glGenerateMipmapEXT;
        glGetFramebufferAttachmentParameteriv = glGetFramebufferAttachmentParameterivEXT;
        glGetRenderbufferParameteriv = glGetRenderbufferParameterivEXT;
        glIsFramebuffer = glIsFramebufferEXT;
        glIsRenderbuffer = glIsRenderbufferEXT;
        glRenderbufferStorage = glRenderbufferStorageEXT;
        glRenderbufferStorageMultisample = glRenderbufferStorageMultisampleEXT;
    }

    // Show the window
    return ShowWindow(__hwnd, SW_SHOW) == 0;
}

//----------------------------------------------------------------
/**
 * @brief Maps a Win32 virtual-key code and shift state to a tractor::Keyboard::Key value.
 * @param win32KeyCode The Win32 virtual-key code (WPARAM) to translate.
 * @param shiftDown True if the Shift key is currently pressed; affects keys that have shifted
 * variants (e.g., digits vs. symbols, letter case).
 * @return The corresponding tractor::Keyboard::Key value for the given key code and shift state.
 * Unhandled or unknown key codes may produce an unspecified result.
 */
tractor::Keyboard::Key getKey(const WPARAM win32KeyCode, bool shiftDown)
{
    using namespace tractor;
    // Structure to hold both shifted and unshifted key mappings
    struct KeyMapping
    {
        Keyboard::Key unshifted;
        Keyboard::Key shifted;

        KeyMapping(Keyboard::Key key) : unshifted(key), shifted(key) {}
        KeyMapping(Keyboard::Key unshifted, Keyboard::Key shifted)
            : unshifted(unshifted), shifted(shifted)
        {
        }
    };

    // Static map initialized once
    static const std::unordered_map<WPARAM, KeyMapping> keyMap = {
        // Function and control keys (no shift variant)
        { VK_PAUSE, KeyMapping(Keyboard::KEY_PAUSE) },
        { VK_SCROLL, KeyMapping(Keyboard::KEY_SCROLL_LOCK) },
        { VK_PRINT, KeyMapping(Keyboard::KEY_PRINT) },
        { VK_ESCAPE, KeyMapping(Keyboard::KEY_ESCAPE) },
        { VK_BACK, KeyMapping(Keyboard::KEY_BACKSPACE) },
        { VK_F16, KeyMapping(Keyboard::KEY_BACKSPACE) }, // CTRL + BACKSPACE
        { VK_TAB, KeyMapping(Keyboard::KEY_TAB, Keyboard::KEY_BACK_TAB) },
        { VK_RETURN, KeyMapping(Keyboard::KEY_RETURN) },
        { VK_CAPITAL, KeyMapping(Keyboard::KEY_CAPS_LOCK) },
        { VK_SHIFT, KeyMapping(Keyboard::KEY_SHIFT) },
        { VK_CONTROL, KeyMapping(Keyboard::KEY_CTRL) },
        { VK_MENU, KeyMapping(Keyboard::KEY_ALT) },
        { VK_APPS, KeyMapping(Keyboard::KEY_MENU) },
        { VK_LSHIFT, KeyMapping(Keyboard::KEY_SHIFT) },
        { VK_RSHIFT, KeyMapping(Keyboard::KEY_SHIFT) },
        { VK_LCONTROL, KeyMapping(Keyboard::KEY_CTRL) },
        { VK_RCONTROL, KeyMapping(Keyboard::KEY_CTRL) },
        { VK_LMENU, KeyMapping(Keyboard::KEY_ALT) },
        { VK_RMENU, KeyMapping(Keyboard::KEY_ALT) },
        { VK_LWIN, KeyMapping(Keyboard::KEY_HYPER) },
        { VK_RWIN, KeyMapping(Keyboard::KEY_HYPER) },
        { VK_BROWSER_SEARCH, KeyMapping(Keyboard::KEY_SEARCH) },

        // Navigation keys
        { VK_INSERT, KeyMapping(Keyboard::KEY_INSERT) },
        { VK_HOME, KeyMapping(Keyboard::KEY_HOME) },
        { VK_PRIOR, KeyMapping(Keyboard::KEY_PG_UP) },
        { VK_DELETE, KeyMapping(Keyboard::KEY_DELETE) },
        { VK_END, KeyMapping(Keyboard::KEY_END) },
        { VK_NEXT, KeyMapping(Keyboard::KEY_PG_DOWN) },
        { VK_LEFT, KeyMapping(Keyboard::KEY_LEFT_ARROW) },
        { VK_RIGHT, KeyMapping(Keyboard::KEY_RIGHT_ARROW) },
        { VK_UP, KeyMapping(Keyboard::KEY_UP_ARROW) },
        { VK_DOWN, KeyMapping(Keyboard::KEY_DOWN_ARROW) },

        // Numpad keys
        { VK_NUMLOCK, KeyMapping(Keyboard::KEY_NUM_LOCK) },
        { VK_ADD, KeyMapping(Keyboard::KEY_KP_PLUS) },
        { VK_SUBTRACT, KeyMapping(Keyboard::KEY_KP_MINUS) },
        { VK_MULTIPLY, KeyMapping(Keyboard::KEY_KP_MULTIPLY) },
        { VK_DIVIDE, KeyMapping(Keyboard::KEY_KP_DIVIDE) },
        { VK_NUMPAD7, KeyMapping(Keyboard::KEY_KP_HOME) },
        { VK_NUMPAD8, KeyMapping(Keyboard::KEY_KP_UP) },
        { VK_NUMPAD9, KeyMapping(Keyboard::KEY_KP_PG_UP) },
        { VK_NUMPAD4, KeyMapping(Keyboard::KEY_KP_LEFT) },
        { VK_NUMPAD5, KeyMapping(Keyboard::KEY_KP_FIVE) },
        { VK_NUMPAD6, KeyMapping(Keyboard::KEY_KP_RIGHT) },
        { VK_NUMPAD1, KeyMapping(Keyboard::KEY_KP_END) },
        { VK_NUMPAD2, KeyMapping(Keyboard::KEY_KP_DOWN) },
        { VK_NUMPAD3, KeyMapping(Keyboard::KEY_KP_PG_DOWN) },
        { VK_NUMPAD0, KeyMapping(Keyboard::KEY_KP_INSERT) },
        { VK_DECIMAL, KeyMapping(Keyboard::KEY_KP_DELETE) },

        // Function keys
        { VK_F1, KeyMapping(Keyboard::KEY_F1) },
        { VK_F2, KeyMapping(Keyboard::KEY_F2) },
        { VK_F3, KeyMapping(Keyboard::KEY_F3) },
        { VK_F4, KeyMapping(Keyboard::KEY_F4) },
        { VK_F5, KeyMapping(Keyboard::KEY_F5) },
        { VK_F6, KeyMapping(Keyboard::KEY_F6) },
        { VK_F7, KeyMapping(Keyboard::KEY_F7) },
        { VK_F8, KeyMapping(Keyboard::KEY_F8) },
        { VK_F9, KeyMapping(Keyboard::KEY_F9) },
        { VK_F10, KeyMapping(Keyboard::KEY_F10) },
        { VK_F11, KeyMapping(Keyboard::KEY_F11) },
        { VK_F12, KeyMapping(Keyboard::KEY_F12) },

        // Space
        { VK_SPACE, KeyMapping(Keyboard::KEY_SPACE) },

        // Number row (with shift variants)
        { 0x30, KeyMapping(Keyboard::KEY_ZERO, Keyboard::KEY_RIGHT_PARENTHESIS) },
        { 0x31, KeyMapping(Keyboard::KEY_ONE, Keyboard::KEY_EXCLAM) },
        { 0x32, KeyMapping(Keyboard::KEY_TWO, Keyboard::KEY_AT) },
        { 0x33, KeyMapping(Keyboard::KEY_THREE, Keyboard::KEY_NUMBER) },
        { 0x34, KeyMapping(Keyboard::KEY_FOUR, Keyboard::KEY_DOLLAR) },
        { 0x35, KeyMapping(Keyboard::KEY_FIVE, Keyboard::KEY_PERCENT) },
        { 0x36, KeyMapping(Keyboard::KEY_SIX, Keyboard::KEY_CIRCUMFLEX) },
        { 0x37, KeyMapping(Keyboard::KEY_SEVEN, Keyboard::KEY_AMPERSAND) },
        { 0x38, KeyMapping(Keyboard::KEY_EIGHT, Keyboard::KEY_ASTERISK) },
        { 0x39, KeyMapping(Keyboard::KEY_NINE, Keyboard::KEY_LEFT_PARENTHESIS) },

        // Punctuation (with shift variants)
        { VK_OEM_PLUS, KeyMapping(Keyboard::KEY_PLUS, Keyboard::KEY_EQUAL) },
        { VK_OEM_COMMA, KeyMapping(Keyboard::KEY_COMMA, Keyboard::KEY_LESS_THAN) },
        { VK_OEM_MINUS, KeyMapping(Keyboard::KEY_MINUS, Keyboard::KEY_UNDERSCORE) },
        { VK_OEM_PERIOD,
          KeyMapping(Keyboard::KEY_PERIOD, Keyboard::KEY_GREATER_THAN) },
        { VK_OEM_1, KeyMapping(Keyboard::KEY_SEMICOLON, Keyboard::KEY_COLON) },
        { VK_OEM_2, KeyMapping(Keyboard::KEY_SLASH, Keyboard::KEY_QUESTION) },
        { VK_OEM_3, KeyMapping(Keyboard::KEY_GRAVE, Keyboard::KEY_TILDE) },
        { VK_OEM_4,
          KeyMapping(Keyboard::KEY_LEFT_BRACKET, Keyboard::KEY_LEFT_BRACE) },
        { VK_OEM_5, KeyMapping(Keyboard::KEY_BACK_SLASH, Keyboard::KEY_BAR) },
        { VK_OEM_6,
          KeyMapping(Keyboard::KEY_RIGHT_BRACKET, Keyboard::KEY_RIGHT_BRACE) },
        { VK_OEM_7, KeyMapping(Keyboard::KEY_APOSTROPHE, Keyboard::KEY_QUOTE) },

        // Letters (with shift variants for capitalization)
        { 0x41, KeyMapping(Keyboard::KEY_A, Keyboard::KEY_CAPITAL_A) },
        { 0x42, KeyMapping(Keyboard::KEY_B, Keyboard::KEY_CAPITAL_B) },
        { 0x43, KeyMapping(Keyboard::KEY_C, Keyboard::KEY_CAPITAL_C) },
        { 0x44, KeyMapping(Keyboard::KEY_D, Keyboard::KEY_CAPITAL_D) },
        { 0x45, KeyMapping(Keyboard::KEY_E, Keyboard::KEY_CAPITAL_E) },
        { 0x46, KeyMapping(Keyboard::KEY_F, Keyboard::KEY_CAPITAL_F) },
        { 0x47, KeyMapping(Keyboard::KEY_G, Keyboard::KEY_CAPITAL_G) },
        { 0x48, KeyMapping(Keyboard::KEY_H, Keyboard::KEY_CAPITAL_H) },
        { 0x49, KeyMapping(Keyboard::KEY_I, Keyboard::KEY_CAPITAL_I) },
        { 0x4A, KeyMapping(Keyboard::KEY_J, Keyboard::KEY_CAPITAL_J) },
        { 0x4B, KeyMapping(Keyboard::KEY_K, Keyboard::KEY_CAPITAL_K) },
        { 0x4C, KeyMapping(Keyboard::KEY_L, Keyboard::KEY_CAPITAL_L) },
        { 0x4D, KeyMapping(Keyboard::KEY_M, Keyboard::KEY_CAPITAL_M) },
        { 0x4E, KeyMapping(Keyboard::KEY_N, Keyboard::KEY_CAPITAL_N) },
        { 0x4F, KeyMapping(Keyboard::KEY_O, Keyboard::KEY_CAPITAL_O) },
        { 0x50, KeyMapping(Keyboard::KEY_P, Keyboard::KEY_CAPITAL_P) },
        { 0x51, KeyMapping(Keyboard::KEY_Q, Keyboard::KEY_CAPITAL_Q) },
        { 0x52, KeyMapping(Keyboard::KEY_R, Keyboard::KEY_CAPITAL_R) },
        { 0x53, KeyMapping(Keyboard::KEY_S, Keyboard::KEY_CAPITAL_S) },
        { 0x54, KeyMapping(Keyboard::KEY_T, Keyboard::KEY_CAPITAL_T) },
        { 0x55, KeyMapping(Keyboard::KEY_U, Keyboard::KEY_CAPITAL_U) },
        { 0x56, KeyMapping(Keyboard::KEY_V, Keyboard::KEY_CAPITAL_V) },
        { 0x57, KeyMapping(Keyboard::KEY_W, Keyboard::KEY_CAPITAL_W) },
        { 0x58, KeyMapping(Keyboard::KEY_X, Keyboard::KEY_CAPITAL_X) },
        { 0x59, KeyMapping(Keyboard::KEY_Y, Keyboard::KEY_CAPITAL_Y) },
        { 0x5A, KeyMapping(Keyboard::KEY_Z, Keyboard::KEY_CAPITAL_Z) },
    };

    // Lookup in the map
    auto it = keyMap.find(win32KeyCode);
    if (it != keyMap.end())
    {
        return shiftDown ? it->second.shifted : it->second.unshifted;
    }

    return Keyboard::KEY_NONE;
}

//----------------------------------------------------------------
void UpdateCapture(const LPARAM lParam)
{
    if ((lParam & MK_LBUTTON) || (lParam & MK_MBUTTON) || (lParam & MK_RBUTTON))
        SetCapture(__hwnd);
    else
        ReleaseCapture();
}

//----------------------------------------------------------------
void WarpMouse(int clientX, int clientY)
{
    POINT p = { clientX, clientY };
    ClientToScreen(__hwnd, &p);
    SetCursorPos(p.x, p.y);
}

//----------------------------------------------------------------
/**
 * Gets the width and height of the screen in pixels.
 */
void getDesktopResolution(int& width, int& height)
{
    RECT desktop;
    const HWND hDesktop = GetDesktopWindow();
    // Get the size of screen to the variable desktop
    GetWindowRect(hDesktop, &desktop);
    width = desktop.right;
    height = desktop.bottom;
}

//----------------------------------------------------------------
LRESULT CALLBACK __WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static tractor::Game* game = tractor::Game::getInstance();

    if (!game->isInitialized() || hwnd != __hwnd)
    {
        // Ignore messages that are not for our game window.
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    static bool shiftDown = false;
    static bool capsOn = false;

    switch (msg)
    {
        case WM_CLOSE:
#ifdef GP_USE_MEM_LEAK_DETECTION
            DestroyWindow(__hwnd);
#else
            exit(0);
#endif
            return 0;

        case WM_DESTROY:
            tractor::Platform::shutdownInternal();
            PostQuitMessage(0);
            return 0;

        case WM_LBUTTONDOWN:
        {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);

            UpdateCapture(wParam);
            if (!tractor::Platform::mouseEventInternal(tractor::Mouse::MOUSE_PRESS_LEFT_BUTTON, x, y, 0))
            {
                tractor::Platform::touchEventInternal(tractor::Touch::TOUCH_PRESS, x, y, 0, true);
            }
            return 0;
        }
        case WM_LBUTTONUP:
        {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);

            if (!tractor::Platform::mouseEventInternal(tractor::Mouse::MOUSE_RELEASE_LEFT_BUTTON,
                                                       x,
                                                       y,
                                                       0))
            {
                tractor::Platform::touchEventInternal(tractor::Touch::TOUCH_RELEASE, x, y, 0, true);
            }
            UpdateCapture(wParam);
            return 0;
        }
        case WM_RBUTTONDOWN:
            UpdateCapture(wParam);
            tractor::Platform::mouseEventInternal(tractor::Mouse::MOUSE_PRESS_RIGHT_BUTTON,
                                                  GET_X_LPARAM(lParam),
                                                  GET_Y_LPARAM(lParam),
                                                  0);
            break;

        case WM_RBUTTONUP:
            tractor::Platform::mouseEventInternal(tractor::Mouse::MOUSE_RELEASE_RIGHT_BUTTON,
                                                  GET_X_LPARAM(lParam),
                                                  GET_Y_LPARAM(lParam),
                                                  0);
            UpdateCapture(wParam);
            break;

        case WM_MBUTTONDOWN:
            UpdateCapture(wParam);
            tractor::Platform::mouseEventInternal(tractor::Mouse::MOUSE_PRESS_MIDDLE_BUTTON,
                                                  GET_X_LPARAM(lParam),
                                                  GET_Y_LPARAM(lParam),
                                                  0);
            break;

        case WM_MBUTTONUP:
            tractor::Platform::mouseEventInternal(tractor::Mouse::MOUSE_RELEASE_MIDDLE_BUTTON,
                                                  GET_X_LPARAM(lParam),
                                                  GET_Y_LPARAM(lParam),
                                                  0);
            UpdateCapture(wParam);
            break;

        case WM_MOUSEMOVE:
        {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);

            if (__mouseCaptured)
            {
                // If the incoming position is the mouse capture point, ignore this event
                // since this is the event that warped the cursor back.
                if (x == __mouseCapturePoint.x && y == __mouseCapturePoint.y) break;

                // Convert to deltas
                x -= __mouseCapturePoint.x;
                y -= __mouseCapturePoint.y;

                // Warp mouse back to center of screen.
                WarpMouse(__mouseCapturePoint.x, __mouseCapturePoint.y);
            }

            // Allow Game::mouseEvent a chance to handle (and possibly consume) the event.
            if (!tractor::Platform::mouseEventInternal(tractor::Mouse::MOUSE_MOVE, x, y, 0))
            {
                if ((wParam & MK_LBUTTON) == MK_LBUTTON)
                {
                    // Mouse move events should be interpreted as touch move only if left mouse is
                    // held and the game did not consume the mouse event.
                    tractor::Platform::touchEventInternal(tractor::Touch::TOUCH_MOVE, x, y, 0, true);
                    return 0;
                }
            }
            break;
        }

        case WM_MOUSEWHEEL:
            tagPOINT point;
            point.x = GET_X_LPARAM(lParam);
            point.y = GET_Y_LPARAM(lParam);
            ScreenToClient(__hwnd, &point);
            tractor::Platform::mouseEventInternal(tractor::Mouse::MOUSE_WHEEL,
                                                  point.x,
                                                  point.y,
                                                  GET_WHEEL_DELTA_WPARAM(wParam)
                                                      / MOUSE_WHEEL_DELTA_DIVISOR);
            break;

        case WM_KEYDOWN:
            if (wParam == VK_SHIFT || wParam == VK_LSHIFT || wParam == VK_RSHIFT) shiftDown = true;

            if (wParam == VK_CAPITAL) capsOn = !capsOn;

            tractor::Platform::keyEventInternal(tractor::Keyboard::KEY_PRESS,
                                                getKey(wParam, shiftDown ^ capsOn));
            break;

        case WM_KEYUP:
            if (wParam == VK_SHIFT || wParam == VK_LSHIFT || wParam == VK_RSHIFT) shiftDown = false;

            tractor::Platform::keyEventInternal(tractor::Keyboard::KEY_RELEASE,
                                                getKey(wParam, shiftDown ^ capsOn));
            break;

        case WM_CHAR:
            tractor::Platform::keyEventInternal(tractor::Keyboard::KEY_CHAR, wParam);
            break;

        case WM_UNICHAR:
            tractor::Platform::keyEventInternal(tractor::Keyboard::KEY_CHAR, wParam);
            break;

        case WM_SETFOCUS:
            break;

        case WM_KILLFOCUS:
            break;

        case WM_SIZE:
            // Window was resized.
            tractor::Platform::resizeEventInternal((unsigned int)(short)LOWORD(lParam),
                                                   (unsigned int)(short)HIWORD(lParam));
            break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

//----------------------------------------------------------------
/**
 * @brief Resolves the initial directory path, using either the provided directory or the current
 * working directory.
 * @param initialDirectory The user-provided initial directory (may be empty).
 * @return The resolved absolute path as a string.
 */
std::string resolveInitialDirectory(const std::string& initialDirectory)
{
    std::vector<char> buffer(MAX_PATH);

    if (initialDirectory.empty())
    {
        GetCurrentDirectoryA(static_cast<DWORD>(buffer.size()), buffer.data());
        return std::string(buffer.data());
    }
    else
    {
        GetFullPathNameA(initialDirectory.c_str(),
                         static_cast<DWORD>(buffer.size()),
                         buffer.data(),
                         nullptr);
        return std::string(buffer.data());
    }
}

//----------------------------------------------------------------
/**
 * @brief Builds the filter string for the file dialog from description and extensions.
 * @param filterDescription The human-readable description of the filter (e.g., "Image Files").
 * @param filterExtensions Semicolon-delimited list of file extensions (e.g., "png;jpg;bmp").
 * @return A pair containing the description string and the double-null-terminated filter buffer.
 */
std::pair<std::string, std::vector<char>> buildFilterString(const std::string& filterDescription,
                                                            const std::string& filterExtensions)
{
    // Build extension string
    std::string extStr;
    std::istringstream f(filterExtensions);
    std::string s;
    bool first = true;

    while (std::getline(f, s, ';'))
    {
        if (!first) extStr += ";";
        extStr += "*.";
        extStr += s;
        first = false;
    }

    // Build description string
    std::ostringstream filterStream;
    filterStream << filterDescription << " (" << extStr << ")";
    std::string descStr = filterStream.str();

    // Create double-null-terminated filter string
    std::vector<char> filter(descStr.size() + extStr.size() + 3);
    std::copy(descStr.begin(), descStr.end(), filter.begin());
    filter[descStr.size()] = '\0';
    std::copy(extStr.begin(), extStr.end(), filter.begin() + descStr.size() + 1);
    filter[descStr.size() + extStr.size() + 1] = '\0';
    filter[descStr.size() + extStr.size() + 2] = '\0';

    return { extStr, std::move(filter) };
}

//----------------------------------------------------------------
/**
 * @brief Populates an OPENFILENAMEA structure with the provided parameters.
 * @param ofn Reference to the OPENFILENAMEA structure to populate.
 * @param szFileName Reference to the filename buffer.
 * @param title Dialog title.
 * @param filter Reference to the filter buffer.
 * @param initialDir Initial directory path.
 * @param defExt Default extension string.
 */
void setupOpenFileName(OPENFILENAMEA& ofn,
                       std::vector<char>& szFileName,
                       const std::string& title,
                       const std::vector<char>& filter,
                       const std::string& initialDir,
                       const std::string& defExt)
{
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetForegroundWindow();
    ofn.lpstrFile = szFileName.data();
    ofn.nMaxFile = static_cast<DWORD>(szFileName.size());
    ofn.lpstrTitle = title.c_str();
    ofn.lpstrFilter = filter.data();
    ofn.lpstrInitialDir = initialDir.c_str();
    ofn.lpstrDefExt = defExt.c_str();
}

//----------------------------------------------------------------
/**
 * @brief Shows the appropriate file dialog (Open or Save) based on the mode.
 * @param mode Dialog mode (FileSystem::OPEN or FileSystem::SAVE).
 * @param ofn Reference to the configured OPENFILENAMEA structure.
 */
void showFileDialog(size_t mode, OPENFILENAMEA& ofn)
{
    if (mode == tractor::FileSystem::OPEN)
    {
        ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
        GetOpenFileNameA(&ofn);
    }
    else
    {
        ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
        GetSaveFileNameA(&ofn);
    }
}

//----------------------------------------------------------------
/**
 * @brief Converts a narrow string to a wide string using Windows MultiByteToWideChar.
 * @param str The narrow string to convert.
 * @return The converted wide string.
 */
std::wstring convertToWideString(const std::string& str)
{
    if (str.empty()) return std::wstring();

    int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
    std::vector<wchar_t> wbuffer(len);
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, wbuffer.data(), len);
    return std::wstring(wbuffer.data());
}

//----------------------------------------------------------------
/**
 * @brief Reads window configuration parameters from the game config.
 * @param game Pointer to the Game instance.
 * @param params Output parameter to populate with window settings.
 */
void loadWindowConfig(const tractor::Game* game, WindowCreationParams& params)
{
    params.fullscreen = false;
    params.resizable = false;
    params.rect = { 0, 0, 0, 0 };
    params.samples = 0;

    if (!game->getConfig()) return;

    tractor::Properties* config = game->getConfig()->getNamespace("window", true);
    if (!config) return;

    // Read window title
    const auto& title = config->getString("title");
    if (!title.empty())
    {
        params.windowName = convertToWideString(title);
    }

    // Read window properties
    params.fullscreen = config->getBool("fullscreen");
    params.resizable = config->getBool("resizable");
    params.samples = config->getInt("samples");

    // Read window position and size
    int x = config->getInt("x");
    if (x != 0) params.rect.left = x;

    int y = config->getInt("y");
    if (y != 0) params.rect.top = y;

    int width = config->getInt("width");
    int height = config->getInt("height");

    // Get desktop resolution for fullscreen if needed
    if (width == 0 && height == 0 && params.fullscreen)
    {
        getDesktopResolution(width, height);
    }

    if (width != 0) params.rect.right = params.rect.left + width;
    if (height != 0) params.rect.bottom = params.rect.top + height;
}

//----------------------------------------------------------------
/**
 * @brief Applies default window dimensions if not specified in config.
 * @param params Window creation parameters to update.
 */
void applyDefaultWindowSize(WindowCreationParams& params)
{
    if (params.rect.right == 0) params.rect.right = params.rect.left + DEFAULT_RESOLUTION_X;

    if (params.rect.bottom == 0) params.rect.bottom = params.rect.top + DEFAULT_RESOLUTION_Y;
}

//----------------------------------------------------------------
/**
 * @brief Validates and potentially adjusts fullscreen display mode.
 * @param width Reference to desired width (may be modified if mode unsupported).
 * @param height Reference to desired height (may be modified if mode unsupported).
 * @param params Window creation parameters (may be modified).
 * @return true if the mode is supported or was successfully adjusted, false otherwise.
 */
bool validateDisplayMode(int& width, int& height, WindowCreationParams& params)
{
    if (!params.fullscreen) return true;

    // Enumerate all display settings to check if mode is supported
    bool modeSupported = false;
    DWORD modeNum = 0;
    DEVMODE devMode{};
    devMode.dmSize = sizeof(DEVMODE);

    while (EnumDisplaySettings(nullptr, modeNum++, &devMode) != 0)
    {
        if (devMode.dmPelsWidth == static_cast<DWORD>(width)
            && devMode.dmPelsHeight == static_cast<DWORD>(height)
            && devMode.dmBitsPerPel == DEFAULT_COLOR_BUFFER_SIZE)
        {
            modeSupported = true;
            break;
        }
    }

    // Fallback to default resolution if not supported
    if (!modeSupported)
    {
        width = DEFAULT_RESOLUTION_X;
        height = DEFAULT_RESOLUTION_Y;
        params.rect.right = params.rect.left + width;
        params.rect.bottom = params.rect.top + height;
    }

    return true;
}

//----------------------------------------------------------------
/**
 * @brief Registers the window class for the application.
 * @return true if registration succeeded, false otherwise.
 */
[[nodiscard]] bool registerWindowClass()
{
    // Get the application module handle
    __hinstance = ::GetModuleHandle(nullptr);

    WNDCLASSEX wc{};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = (WNDPROC)__WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = __hinstance;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hIconSm = nullptr;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = L"gameplay";

    if (!::RegisterClassEx(&wc))
    {
        GP_ERROR("Failed to register window class.");
        return false;
    }

    return true;
}

//----------------------------------------------------------------
/**
 * @brief Sets the display mode to fullscreen with specified dimensions.
 * @param width Desired screen width.
 * @param height Desired screen height.
 * @return true if display settings were applied successfully, false otherwise.
 */
[[nodiscard]] bool setFullscreenMode(int width, int height)
{
    DEVMODE dm{};
    dm.dmSize = sizeof(dm);
    dm.dmPelsWidth = width;
    dm.dmPelsHeight = height;
    dm.dmBitsPerPel = DEFAULT_COLOR_BUFFER_SIZE;
    dm.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

    if (ChangeDisplaySettings(&dm, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
    {
        GP_ERROR("Failed to start game in full-screen mode with resolution %dx%d.", width, height);
        return false;
    }

    return true;
}

#ifdef GP_USE_GAMEPAD
//----------------------------------------------------------------
/**
 * @brief Initializes and detects connected XInput gamepads.
 */
void initializeGamepads()
{
    for (DWORD i = 0; i < XUSER_MAX_COUNT; i++)
    {
        if (XInputGetState(i, &__xInputState) == NO_ERROR)
        {
            if (!__connectedXInput[i])
            {
                tractor::Platform::gamepadEventConnectedInternal(i,
                                                                 XINPUT_BUTTON_COUNT,
                                                                 XINPUT_JOYSTICK_COUNT,
                                                                 XINPUT_TRIGGER_COUNT,
                                                                 "Microsoft XBox360 Controller");
                __connectedXInput[i] = true;
            }
        }
    }
}

//----------------------------------------------------------------
/**
 * @brief Polls XInput gamepads for connection/disconnection events.
 */
void pollGamepadConnections()
{
    for (DWORD i = 0; i < XUSER_MAX_COUNT; i++)
    {
        const DWORD result = XInputGetState(i, &__xInputState);

        if (result == NO_ERROR && !__connectedXInput[i])
        {
            // Gamepad was just connected
            tractor::Platform::gamepadEventConnectedInternal(i,
                                                             XINPUT_BUTTON_COUNT,
                                                             XINPUT_JOYSTICK_COUNT,
                                                             XINPUT_TRIGGER_COUNT,
                                                             "Microsoft XBox360 Controller");
            __connectedXInput[i] = true;
        }
        else if (result != NO_ERROR && __connectedXInput[i])
        {
            // Gamepad was just disconnected
            __connectedXInput[i] = false;
            tractor::Platform::gamepadEventDisconnectedInternal(i);
        }
    }
}
//----------------------------------------------------------------
/**
 * @brief Maps XInput button bits to Gamepad button flags.
 * @param buttons Raw XInput button state (WORD).
 * @return Mapped button flags compatible with Gamepad::ButtonMappings.
 */
unsigned int mapXInputButtons(WORD buttons)
{
    // Map XInput buttons to Gamepad::ButtonMappings enum
    static constexpr unsigned int xInputMapping[16] = {
        tractor::Gamepad::BUTTON_UP,    // 0x0001
        tractor::Gamepad::BUTTON_DOWN,  // 0x0002
        tractor::Gamepad::BUTTON_LEFT,  // 0x0004
        tractor::Gamepad::BUTTON_RIGHT, // 0x0008
        tractor::Gamepad::BUTTON_MENU2, // 0x0010
        tractor::Gamepad::BUTTON_MENU1, // 0x0020
        tractor::Gamepad::BUTTON_L3,    // 0x0040
        tractor::Gamepad::BUTTON_R3,    // 0x0080
        tractor::Gamepad::BUTTON_L1,    // 0x0100
        tractor::Gamepad::BUTTON_R1,    // 0x0200
        0,                              // 0x0400 (unused)
        0,                              // 0x0800 (unused)
        tractor::Gamepad::BUTTON_A,     // 0x1000
        tractor::Gamepad::BUTTON_B,     // 0x2000
        tractor::Gamepad::BUTTON_X,     // 0x4000
        tractor::Gamepad::BUTTON_Y      // 0x8000
    };

    unsigned int mappedButtons = 0;
    for (unsigned int bit = 0; bit < 16; ++bit)
    {
        if ((buttons & (1 << bit)) && xInputMapping[bit] != 0)
        {
            mappedButtons |= (1 << xInputMapping[bit]);
        }
    }

    return mappedButtons;
}

//----------------------------------------------------------------
/**
 * @brief Normalizes a trigger value with dead zone applied.
 * @param triggerValue Raw trigger value (0-255).
 * @return Normalized trigger value (0.0-1.0).
 */
float normalizeXInputTrigger(BYTE triggerValue)
{
    if (triggerValue < XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
    {
        return 0.0f;
    }

    return static_cast<float>(triggerValue) / 255.0f;
}

//----------------------------------------------------------------
/**
 * @brief Updates joystick values for a gamepad from XInput state.
 * @param gamepad The gamepad to update.
 * @param state The current XInput state.
 */
void updateGamepadJoysticks(tractor::Gamepad* gamepad, const XINPUT_STATE& state)
{
    auto joystickCount = gamepad->getJoystickCount();

    // Left joystick (index 0)
    if (joystickCount > 0)
    {
        const float leftX =
            normalizeXInputJoystickAxis(state.Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
        const float leftY =
            normalizeXInputJoystickAxis(state.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
        gamepad->setJoystickValue(0, leftX, leftY);
    }

    // Right joystick (index 1)
    if (joystickCount > 1)
    {
        const float rightX =
            normalizeXInputJoystickAxis(state.Gamepad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
        const float rightY =
            normalizeXInputJoystickAxis(state.Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
        gamepad->setJoystickValue(1, rightX, rightY);
    }
}

//----------------------------------------------------------------
/**
 * @brief Updates trigger values for a gamepad from XInput state.
 * @param gamepad The gamepad to update.
 * @param state The current XInput state.
 */
void updateGamepadTriggers(tractor::Gamepad* gamepad, const XINPUT_STATE& state)
{
    auto triggerCount = gamepad->getTriggerCount();

    // Left trigger (index 0)
    if (triggerCount > 0)
    {
        const float leftTrigger = normalizeXInputTrigger(state.Gamepad.bLeftTrigger);
        gamepad->setTriggerValue(0, leftTrigger);
    }

    // Right trigger (index 1)
    if (triggerCount > 1)
    {
        const float rightTrigger = normalizeXInputTrigger(state.Gamepad.bRightTrigger);
        gamepad->setTriggerValue(1, rightTrigger);
    }
}
#endif

//----------------------------------------------------------------
/**
 * @brief Initializes the high-resolution performance timer.
 */
void initializePerformanceTimer()
{
    LARGE_INTEGER tps;
    QueryPerformanceFrequency(&tps);
    __timeTicksPerMillis = static_cast<double>(tps.QuadPart / 1000L);

    LARGE_INTEGER queryTime;
    QueryPerformanceCounter(&queryTime);

    assert(__timeTicksPerMillis > 0.0);
    __timeStart = queryTime.QuadPart / __timeTicksPerMillis;
}

//----------------------------------------------------------------
/**
 * @brief Processes a single Windows message.
 * @param msg The message to process.
 * @return True if the application should quit, false otherwise.
 */
bool processWindowsMessage(int& result)
{
    MSG msg{};

    if (!PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) return false;

    TranslateMessage(&msg);
    DispatchMessage(&msg);

    result = static_cast<int>(msg.wParam);

    if (msg.message == WM_QUIT)
    {
        tractor::Platform::shutdownInternal();
        return true;
    }

    return false;
}

//----------------------------------------------------------------
/**
 * @brief Renders a single frame and handles gamepad input.
 * @param game Pointer to the game instance.
 */
void renderFrame(tractor::Game* game)
{
#ifdef GP_USE_GAMEPAD
    pollGamepadConnections();
#endif
    game->frame();
    SwapBuffers(__hdc);
}

} // namespace

namespace tractor
{
/**
 * @brief Windows implementation of print function that outputs formatted text to both stderr and the Visual Studio debug output.
 * @param format 
 * @param  
 */
extern void print(const char* format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    int sz = vfprintf(stderr, format, argptr);
    if (sz > 0)
    {
        char* buf = new char[sz + 1];
        vsprintf(buf, format, argptr);
        buf[sz] = 0;
        OutputDebugStringA(buf);
        SAFE_DELETE_ARRAY(buf);
    }
    va_end(argptr);
}

//----------------------------------------------------------------
// Platform class
//----------------------------------------------------------------
Platform::Platform(Game* game) : _game(game) {}

//----------------------------------------------------------------
Platform::~Platform()
{
    if (__hwnd)
    {
        DestroyWindow(__hwnd);
        __hwnd = 0;
    }
}

//----------------------------------------------------------------
Platform* Platform::create(Game* game)
{
    assert(game);

    FileSystem::setResourcePath("./");

    // Custom deleter that calls exit(0) on destruction
    auto deleter = [](Platform* p)
    {
        delete p;
        exit(0);
    };
    std::unique_ptr<Platform, decltype(deleter)> platform(new Platform(game), deleter);

    // Read and configure window parameters
    WindowCreationParams params;
    loadWindowConfig(game, params);
    applyDefaultWindowSize(params);

    // Calculate window dimensions
    int width = params.rect.right - params.rect.left;
    int height = params.rect.bottom - params.rect.top;

    if (!validateDisplayMode(width, height, params))
        return nullptr;

    if (!registerWindowClass())
        return nullptr;

    if (params.fullscreen)
        params.fullscreen = setFullscreenMode(width, height);

    // Initialize OpenGL
    if (!initializeGL(&params))
        return nullptr;

#ifdef GP_USE_GAMEPAD
    // Initialize gamepads
    initializeGamepads();
#endif

    return platform.release();
}

//----------------------------------------------------------------
int Platform::enterMessagePump()
{
    assert(_game);

    // Initialize high-resolution timer
    initializePerformanceTimer();

    // Initial buffer swap
    SwapBuffers(__hdc);

    // Start the game if not already running
    if (_game->getState() != Game::RUNNING)
        _game->run();

    // Main message loop
    int result = 0;
    while (_game->getState() != Game::UNINITIALIZED)
    {
        if (processWindowsMessage(result))
            return result;// processed windows message
        else
            renderFrame(_game);// Render frame when no messages are pending
    }

    return 0;
}

//----------------------------------------------------------------
void Platform::signalShutdown()
{
    // nothing to do
}

//----------------------------------------------------------------
bool Platform::canExit() { return true; }

//----------------------------------------------------------------
unsigned int Platform::getDisplayWidth()
{
    static RECT rect;
    GetClientRect(__hwnd, &rect);
    return rect.right;
}

//----------------------------------------------------------------
unsigned int Platform::getDisplayHeight()
{
    static RECT rect;
    GetClientRect(__hwnd, &rect);
    return rect.bottom;
}

//----------------------------------------------------------------
double Platform::getAbsoluteTime()
{
    LARGE_INTEGER queryTime;
    QueryPerformanceCounter(&queryTime);
    assert(__timeTicksPerMillis);
    __timeAbsolute = queryTime.QuadPart / __timeTicksPerMillis;

    return __timeAbsolute - __timeStart;
}

//----------------------------------------------------------------
void Platform::setAbsoluteTime(double time) { __timeAbsolute = time; }

//----------------------------------------------------------------
bool Platform::isVsync() { return __vsync; }

//----------------------------------------------------------------
void Platform::setVsync(bool enable)
{
    __vsync = enable;

    if (wglSwapIntervalEXT)
        wglSwapIntervalEXT(__vsync ? 1 : 0);
    else
        __vsync = false;
}

void Platform::swapBuffers()
{
    if (__hdc) SwapBuffers(__hdc);
}

void Platform::sleep(long ms) { Sleep(ms); }

//----------------------------------------------------------------
void Platform::setMultiSampling(bool enabled)
{
    if (enabled == __multiSampling)
    {
        return;
    }

    if (enabled)
    {
        glEnable(GL_MULTISAMPLE);
    }
    else
    {
        glDisable(GL_MULTISAMPLE);
    }

    __multiSampling = enabled;
}

//----------------------------------------------------------------
bool Platform::isMultiSampling() { return __multiSampling; }

//----------------------------------------------------------------
void Platform::setMultiTouch(bool enabled)
{
    // not supported
}

//----------------------------------------------------------------
bool Platform::isMultiTouch() { return false; }

//----------------------------------------------------------------
bool Platform::hasAccelerometer() { return false; }

//----------------------------------------------------------------
void Platform::getAccelerometerValues(float* pitch, float* roll)
{
    assert(pitch);
    assert(roll);

    *pitch = 0;
    *roll = 0;
}

//----------------------------------------------------------------
void Platform::getSensorValues(float* accelX,
                               float* accelY,
                               float* accelZ,
                               float* gyroX,
                               float* gyroY,
                               float* gyroZ)
{
    if (accelX) *accelX = 0;
    if (accelY) *accelY = 0;
    if (accelZ) *accelZ = 0;
    if (gyroX) *gyroX = 0;
    if (gyroY) *gyroY = 0;
    if (gyroZ) *gyroZ = 0;
}

//----------------------------------------------------------------
void Platform::getArguments(int* argc, char*** argv)
{
    if (argc) *argc = __argc;
    if (argv) *argv = __argv;
}

//----------------------------------------------------------------
bool Platform::hasMouse() { return true; }

//----------------------------------------------------------------
void Platform::setMouseCaptured(bool captured)
{
    if (captured != __mouseCaptured)
    {
        if (captured)
        {
            // Hide the cursor and warp it to the center of the screen
            __mouseCapturePoint.x = getDisplayWidth() / 2;
            __mouseCapturePoint.y = getDisplayHeight() / 2;

            ShowCursor(FALSE);
            WarpMouse(__mouseCapturePoint.x, __mouseCapturePoint.y);
        }
        else
        {
            // Restore cursor
            WarpMouse(__mouseCapturePoint.x, __mouseCapturePoint.y);
            ShowCursor(TRUE);
        }

        __mouseCaptured = captured;
    }
}

//----------------------------------------------------------------
bool Platform::isMouseCaptured() { return __mouseCaptured; }

//----------------------------------------------------------------
void Platform::setCursorVisible(bool visible)
{
    if (visible != __cursorVisible)
    {
        ShowCursor(visible ? TRUE : FALSE);
        __cursorVisible = visible;
    }
}

//----------------------------------------------------------------
bool Platform::isCursorVisible() { return __cursorVisible; }

//----------------------------------------------------------------
void Platform::displayKeyboard(bool display)
{
    // Do nothing.
}

//----------------------------------------------------------------
bool Platform::isGestureSupported(Gesture::GestureEvent evt) { return false; }

//----------------------------------------------------------------
void Platform::registerGesture(Gesture::GestureEvent evt) {}

//----------------------------------------------------------------
void Platform::unregisterGesture(Gesture::GestureEvent evt) {}

//----------------------------------------------------------------
bool Platform::isGestureRegistered(Gesture::GestureEvent evt) { return false; }

#ifdef GP_USE_GAMEPAD
//----------------------------------------------------------------
void Platform::pollGamepadState(Gamepad* gamepad)
{
    assert(gamepad);
    assert(gamepad->_handle < XUSER_MAX_COUNT);

    XINPUT_STATE state;
    if (XInputGetState(gamepad->_handle, &state) == NO_ERROR)
    {
        // Update buttons
        const unsigned int mappedButtons = mapXInputButtons(state.Gamepad.wButtons);
        gamepad->setButtons(mappedButtons);

        // Update joysticks
        updateGamepadJoysticks(gamepad, state);

        // Update triggers
        updateGamepadTriggers(gamepad, state);
    }
}
#else
void Platform::pollGamepadState(Gamepad* gamepad) {}
#endif

//----------------------------------------------------------------
void Platform::shutdownInternal() { Game::getInstance()->shutdown(); }

//----------------------------------------------------------------
bool Platform::launchURL(const std::string& url)
{
    if (url.empty()) return false;
    auto urlPtr = url.c_str();

    // Convert to wide string
    int len = MultiByteToWideChar(CP_ACP, 0, url.c_str(), -1, nullptr, 0);
    std::vector<wchar_t> wurl(len);
    MultiByteToWideChar(CP_ACP, 0, url.c_str(), -1, wurl.data(), len);

    INT_PTR r = reinterpret_cast<INT_PTR>(
        ShellExecute(nullptr, nullptr, wurl.data(), nullptr, nullptr, SW_SHOWNORMAL));

    return (r > SHELL_EXECUTE_SUCCESS_THRESHOLD);
}

//----------------------------------------------------------------
std::string Platform::displayFileDialog(size_t mode,
                                        const std::string& title,
                                        const std::string& filterDescription,
                                        const std::string& filterExtensions,
                                        const std::string& initialDirectory)
{
    OPENFILENAMEA ofn{};

    // Resolve initial directory
    std::string initialDirectoryStr = resolveInitialDirectory(initialDirectory);

    // Build filter string
    auto [extStr, filter] = buildFilterString(filterDescription, filterExtensions);

    // Prepare filename buffer
    std::vector<char> szFileName(MAX_PATH, '\0');

    // Setup OPENFILENAMEA structure
    setupOpenFileName(ofn, szFileName, title, filter, initialDirectoryStr, extStr);

    // Show the dialog
    showFileDialog(mode, ofn);

    return std::string(szFileName.data());
}

} // namespace tractor
