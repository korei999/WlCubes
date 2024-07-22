#include <windowsx.h>

#include "input.hh"
#include "logs.hh"
#include "../../frame.hh"

#include <hidusage.h>
#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC         ((USHORT) 0x01)
#endif
#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE        ((USHORT) 0x02)
#endif

namespace win32
{
namespace input
{

LRESULT CALLBACK
windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Window* pWin = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    RAWINPUTDEVICE Rid[1];
    Rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    Rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
    Rid[0].dwFlags = RIDEV_INPUTSINK;
    Rid[0].hwndTarget = hwnd;
    RegisterRawInputDevices(Rid, 1, sizeof(Rid[0]));

    switch (msg)
    {
        case WM_DESTROY:
            LOG_OK("WM_DESTROY\n");
            PostQuitMessage(0);
            return 0;

        case WM_SIZE:
            pWin->wWidth = LOWORD(lParam);
            pWin->wHeight = HIWORD(lParam);
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
                        controls::procKeysOnce(pWin, KEY_Q, isUp);
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

                frame::player.mouse.relX = GET_X_LPARAM(lParam);
                frame::player.mouse.relY = GET_Y_LPARAM(lParam);
            }
            break;

        // case WM_INPUT:
        //     {
        //         UINT dwSize = sizeof(RAWINPUT);
        //         static BYTE lpb[sizeof(RAWINPUT)];

        //         GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));

        //         RAWINPUT* raw = (RAWINPUT*)lpb;

        //         if (raw->header.dwType == RIM_TYPEMOUSE) 
        //         {
        //             frame::player.mouse.relX += raw->data.mouse.lLastX;
        //             frame::player.mouse.relY += raw->data.mouse.lLastY;
        //         } 
        //     }
        //     break;

        default:
            break;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} /* namespace input */
} /* namespace win32 */
