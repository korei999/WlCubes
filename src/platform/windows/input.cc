#include <windowsx.h>

#include "input.hh"
#include "logs.hh"
#include "../../frame.hh"

namespace win32
{
namespace input
{

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

                frame::player.mouse.relX = GET_X_LPARAM(lParam);
                frame::player.mouse.relY = GET_Y_LPARAM(lParam);
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
