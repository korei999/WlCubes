#include <windowsx.h>

#include "input.hh"
#include "logs.hh"
#include "../../frame.hh"

namespace win32
{
namespace input
{

int asciiToLinuxKeyCodes[300] {
    [0] = 0,
    [1] = 0,
    [2] = 0,
    [3] = 0,
    [4] = 0,
    [5] = 0,
    [6] = 0,
    [7] = 0,
    [8] = 0,
    [9] = 0,
    [10] = 0,
    [11] = KEY_TAB,
    [12] = 0,
    [13] = 0,
    [14] = 0,
    [15] = 0,
    [16] = KEY_LEFTSHIFT,
    [17] = KEY_LEFTCTRL,
    [18] = 0,
    [19] = 0,
    [20] = 0,
    [21] = 0,
    [22] = 0,
    [23] = 0,
    [24] = 0,
    [25] = 0,
    [26] = 0,
    [27] = KEY_ESC,
    [28] = 0,
    [29] = 0,
    [30] = 0,
    [31] = 0,
    [32] = KEY_SPACE,
    ['!'] = KEY_1,
    ['"'] = KEY_APOSTROPHE,
    ['#'] = KEY_3,
    ['$'] = KEY_4,
    ['%'] = KEY_5,
    ['&'] = KEY_7,
    ['\''] = KEY_APOSTROPHE,
    ['('] = KEY_9,
    [')'] = KEY_0,
    ['*'] = KEY_8,
    ['+'] = KEY_EQUAL,
    [','] = KEY_COMMA,
    ['-'] = KEY_MINUS,
    ['.'] = KEY_DOT,
    ['/'] = KEY_SLASH,
    ['0'] = KEY_0,
    ['1'] = KEY_1,
    ['2'] = KEY_2,
    ['3'] = KEY_3,
    ['4'] = KEY_4,
    ['5'] = KEY_5,
    ['6'] = KEY_6,
    ['7'] = KEY_7,
    ['8'] = KEY_8,
    ['9'] = KEY_9,
    [':'] = KEY_SEMICOLON,
    [';'] = KEY_SEMICOLON,
    ['<'] = KEY_COMMA,
    ['='] = KEY_EQUAL,
    ['>'] = KEY_DOT,
    ['?'] = KEY_SLASH,
    ['@'] = KEY_2,
    ['A'] = KEY_A,
    ['B'] = KEY_B,
    ['C'] = KEY_C,
    ['D'] = KEY_D,
    ['E'] = KEY_E,
    ['F'] = KEY_F,
    ['G'] = KEY_G,
    ['H'] = KEY_H,
    ['I'] = KEY_I,
    ['J'] = KEY_J,
    ['K'] = KEY_K,
    ['L'] = KEY_L,
    ['M'] = KEY_M,
    ['N'] = KEY_N,
    ['O'] = KEY_O,
    ['P'] = KEY_O,
    ['Q'] = KEY_Q,
    ['R'] = KEY_R,
    ['S'] = KEY_S,
    ['T'] = KEY_T,
    ['U'] = KEY_U,
    ['V'] = KEY_V,
    ['W'] = KEY_W,
    ['X'] = KEY_X,
    ['Y'] = KEY_Y,
    ['Z'] = KEY_Z,
    ['['] = KEY_LEFTBRACE,
    ['\\'] = KEY_BACKSLASH,
    [']'] = KEY_RIGHTBRACE,
    ['^'] = KEY_6,
    ['_'] = KEY_MINUS,
    ['`'] = KEY_GRAVE,
    ['a'] = KEY_A,
    ['b'] = KEY_B,
    ['c'] = KEY_C,
    ['d'] = KEY_D,
    ['e'] = KEY_E,
    ['f'] = KEY_F,
    ['g'] = KEY_G,
    ['h'] = KEY_H,
    ['i'] = KEY_I,
    ['j'] = KEY_J,
    ['k'] = KEY_K,
    ['l'] = KEY_L,
    ['m'] = KEY_M,
    ['n'] = KEY_N,
    ['o'] = KEY_O,
    ['p'] = KEY_O,
    ['q'] = KEY_Q,
    ['r'] = KEY_R,
    ['s'] = KEY_S,
    ['t'] = KEY_T,
    ['u'] = KEY_U,
    ['v'] = KEY_V,
    ['w'] = KEY_W,
    ['x'] = KEY_X,
    ['y'] = KEY_Y,
    ['z'] = KEY_Z,
    ['{'] = KEY_LEFTBRACE,
    ['|'] = KEY_BACKSLASH,
    ['}'] = KEY_RIGHTBRACE,
    ['~'] = KEY_GRAVE,
    [127] = KEY_DELETE,
};

/* https://gist.github.com/luluco250/ac79d72a734295f167851ffdb36d77ee */

void
registerRawMouseDevice(Window* self, bool on)
{
    DWORD flag = on ? RIDEV_NOLEGACY : RIDEV_REMOVE;

    self->rawInputDevices[0].usUsagePage = 0x01; /* HID_USAGE_PAGE_GENERIC */
    self->rawInputDevices[0].usUsage = 0x02;     /* HID_USAGE_GENERIC_MOUSE */
    self->rawInputDevices[0].dwFlags = flag;     /* adds mouse and also ignores legacy mouse messages */
    self->rawInputDevices[0].hwndTarget = 0;

    // self->rawInputDevices[1].usUsagePage = 0x01;       /* HID_USAGE_PAGE_GENERIC */
    // self->rawInputDevices[1].usUsage = 0x06;           /* HID_USAGE_GENERIC_KEYBOARD */
    // self->rawInputDevices[1].dwFlags = RIDEV_NOLEGACY; /* adds keyboard and also ignores legacy keyboard messages */
    // self->rawInputDevices[1].hwndTarget = 0;

    if (RegisterRawInputDevices(self->rawInputDevices, 1, sizeof(self->rawInputDevices[0])) == FALSE)
        LOG_FATAL("RegisterRawInputDevices failed: %lu\n", GetLastError());
}

void
registerRawKBDevice(Window* self, bool on)
{
    DWORD flag = on ? RIDEV_NOLEGACY : RIDEV_REMOVE;

    self->rawInputDevices[1].usUsagePage = 0x01;       /* HID_USAGE_PAGE_GENERIC */
    self->rawInputDevices[1].usUsage = 0x06;           /* HID_USAGE_GENERIC_KEYBOARD */
    self->rawInputDevices[1].dwFlags = RIDEV_NOLEGACY; /* adds keyboard and also ignores legacy keyboard messages */
    self->rawInputDevices[1].hwndTarget = 0;

    if (RegisterRawInputDevices(self->rawInputDevices, 1, sizeof(self->rawInputDevices[1])) == FALSE)
        LOG_FATAL("RegisterRawInputDevices failed: %lu\n", GetLastError());
}

bool
enterFullscreen(HWND hwnd, int fullscreenWidth, int fullscreenHeight, int colourBits, int refreshRate)
{
    DEVMODE fullscreenSettings;
    bool isChangeSuccessful;
    RECT windowBoundary;

    EnumDisplaySettings(NULL, 0, &fullscreenSettings);
    fullscreenSettings.dmPelsWidth        = fullscreenWidth;
    fullscreenSettings.dmPelsHeight       = fullscreenHeight;
    fullscreenSettings.dmBitsPerPel       = colourBits;
    fullscreenSettings.dmDisplayFrequency = refreshRate;
    fullscreenSettings.dmFields           = DM_PELSWIDTH |
                                            DM_PELSHEIGHT |
                                            DM_BITSPERPEL |
                                            DM_DISPLAYFREQUENCY;

    SetWindowLongPtr(hwnd, GWL_EXSTYLE, WS_EX_APPWINDOW | WS_EX_TOPMOST);
    SetWindowLongPtr(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, fullscreenWidth, fullscreenHeight, SWP_SHOWWINDOW);
    isChangeSuccessful = ChangeDisplaySettings(&fullscreenSettings, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL;
    ShowWindow(hwnd, SW_MAXIMIZE);

    return isChangeSuccessful;
}

bool
exitFullscreen(HWND hwnd, int windowX, int windowY, int windowedWidth, int windowedHeight, int windowedPaddingX, int windowedPaddingY)
{
    bool isChangeSuccessful;

    SetWindowLongPtr(hwnd, GWL_EXSTYLE, WS_EX_LEFT);
    SetWindowLongPtr(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
    isChangeSuccessful = ChangeDisplaySettings(NULL, CDS_RESET) == DISP_CHANGE_SUCCESSFUL;
    SetWindowPos(hwnd, HWND_NOTOPMOST, windowX, windowY, windowedWidth + windowedPaddingX, windowedHeight + windowedPaddingY, SWP_SHOWWINDOW);
    ShowWindow(hwnd, SW_RESTORE);

    return isChangeSuccessful;
}

LRESULT CALLBACK
windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Window* self = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg)
    {
        case WM_DESTROY:
            self->bRunning = false;
            return 0;

        case WM_SIZE:
            self->wWidth = LOWORD(lParam);
            self->wHeight = HIWORD(lParam);
            break;

        case WM_NCCREATE:
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCT*)lParam)->lpCreateParams);
            break;

        case WM_LBUTTONDOWN:
            break;

        case WM_KEYUP:
        case WM_KEYDOWN:
            {
                WPARAM keyCode = wParam;
                bool isUp = !((lParam >> 31) & 1);

                controls::pressedKeys[ asciiToLinuxKeyCodes[keyCode] ] = isUp;
                controls::procKeysOnce(self, asciiToLinuxKeyCodes[keyCode], isUp);
            }
            break;

        case WM_MOUSEMOVE:
            {
                frame::player.mouse.absX = GET_X_LPARAM(lParam);
                frame::player.mouse.absY = GET_Y_LPARAM(lParam);
            }
            break;

        case WM_INPUT:
            {
                u32 size = sizeof(RAWINPUT);
                static RAWINPUT raw[sizeof(RAWINPUT)];
                GetRawInputData((HRAWINPUT)lParam, RID_INPUT, raw, &size, sizeof(RAWINPUTHEADER));

                if (raw->header.dwType == RIM_TYPEMOUSE)
                {
                    frame::player.mouse.relX += raw->data.mouse.lLastX;
                    frame::player.mouse.relY += raw->data.mouse.lLastY;
                }
                // if (raw->header.dwType == RIM_TYPEKEYBOARD)
                // {
                //     auto key = raw->data.keyboard.VKey;
                //     /* 0 == down, 1 == up */
                //     auto flags = raw->data.keyboard.Flags;
                //     COUT("key: %u, flags: %u\n", key, flags);

                //     if (key < 300)
                //     {
                //         controls::pressedKeys[key - 35] = !flags;
                //     }
                // }
            }
            break;

        default:
            break;
    }

    if (self && self->bRelativeMode)
    {
        SetCursorPos(self->wWidth / 2, self->wHeight / 2);
        SetCursor(nullptr);
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} /* namespace input */
} /* namespace win32 */
