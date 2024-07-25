#include <windowsx.h>

#include "input.hh"
#include "logs.hh"
#include "../../frame.hh"

namespace win32
{
namespace input
{

/* https://gist.github.com/luluco250/ac79d72a734295f167851ffdb36d77ee */

void
registerRawMouseDevice(Window* self, bool on)
{
    DWORD flag = on ? RIDEV_NOLEGACY : RIDEV_REMOVE;

    self->rawInputDevices[0].usUsagePage = 0x01; /* HID_USAGE_PAGE_GENERIC */
    self->rawInputDevices[0].usUsage = 0x02;     /* HID_USAGE_GENERIC_MOUSE */
    self->rawInputDevices[0].dwFlags = flag;     /* adds mouse and also ignores legacy mouse messages */
    self->rawInputDevices[0].hwndTarget = 0;

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

    if (RegisterRawInputDevices(self->rawInputDevices, 1, sizeof(self->rawInputDevices[0])) == FALSE)
        LOG_FATAL("RegisterRawInputDevices failed: %lu\n", GetLastError());
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
            }
            break;

        default:
            break;
    }

    if (self && self->bRelativeMode) SetCursorPos(self->wWidth / 2, self->wHeight / 2);

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} /* namespace input */
} /* namespace win32 */
