#include <windowsx.h>

#include "input.hh"
#include "logs.hh"
#include "../../frame.hh"

namespace win32
{
namespace input
{

int asciiToLinuxKeyCodes[] {
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
                COUT("keycode: %llu, up: %d\n", keyCode, isUp);
                switch (keyCode)
                {
                    case 'W':
                        controls::pressedKeys[KEY_W] = isUp;
                        break;

                    case 'A':
                        controls::pressedKeys[KEY_A] = isUp;
                        break;

                    case 'S':
                        controls::pressedKeys[KEY_S] = isUp;
                        break;

                    case 'D':
                        controls::pressedKeys[KEY_D] = isUp;
                        break;

                    case ' ':
                        controls::pressedKeys[KEY_SPACE] = isUp;
                        break;

                    case VK_CONTROL:
                        controls::pressedKeys[KEY_LEFTCTRL] = isUp;
                        break;

                    case 'Q':
                        controls::pressedKeys[KEY_Q] = isUp;
                        controls::procKeysOnce(self, KEY_Q, isUp);
                        break;

                    case 'F':
                        controls::pressedKeys[KEY_F] = isUp;
                        controls::procKeysOnce(self, KEY_F, isUp);
                        break;

                    case 27: /* esc */
                        self->bRunning = false;
                        break;

                    default:
                        break;
                };
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
