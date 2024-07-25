#include <windowsx.h>

#include "input.hh"
#include "logs.hh"
#include "../../frame.hh"

namespace win32
{
namespace input
{

void
initRawDevices(Window* self)
{
    RAWINPUTDEVICE Rid[2];

    Rid[0].usUsagePage = 0x01;          // HID_USAGE_PAGE_GENERIC
    Rid[0].usUsage = 0x02;              // HID_USAGE_GENERIC_MOUSE
    Rid[0].dwFlags = RIDEV_NOLEGACY;    // adds mouse and also ignores legacy mouse messages
    Rid[0].hwndTarget = 0;

    // Rid[1].usUsagePage = 0x01;          // HID_USAGE_PAGE_GENERIC
    // Rid[1].usUsage = 0x06;              // HID_USAGE_GENERIC_KEYBOARD
    // Rid[1].dwFlags = RIDEV_NOLEGACY;    // adds keyboard and also ignores legacy keyboard messages
    // Rid[1].hwndTarget = 0;

    if (RegisterRawInputDevices(Rid, 1, sizeof(Rid[0])) == FALSE)
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

        // case WM_MOUSEMOVE:
        //     {
        //         frame::player.mouse.absX = GET_X_LPARAM(lParam);
        //         frame::player.mouse.absY = GET_Y_LPARAM(lParam);

        //         frame::player.mouse.relX = GET_X_LPARAM(lParam);
        //         frame::player.mouse.relY = GET_Y_LPARAM(lParam);
        //     }
        //     break;

        case WM_INPUT:
            {
                u32 size = sizeof(RAWINPUT);
                static RAWINPUT raw[sizeof(RAWINPUT)];
                GetRawInputData((HRAWINPUT)lParam, RID_INPUT, raw, &size, sizeof(RAWINPUTHEADER));

                if (raw->header.dwType == RIM_TYPEMOUSE) {
                    frame::player.mouse.relX += raw->data.mouse.lLastX;
                    frame::player.mouse.relY += raw->data.mouse.lLastY;

                    // if (raw->data.mouse.usButtonFlags & RI_MOUSE_WHEEL)
                    //     input.mouse.wheel = (*(short*)&raw->data.mouse.usButtonData) / WHEEL_DELTA;
                }
            }
            break;

        default:
            break;
    }

    // detectInput(self);

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} /* namespace input */
} /* namespace win32 */
