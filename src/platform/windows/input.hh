#pragma once
#include "windows.hh"

namespace win32
{
namespace input
{

bool initDirectInput(HWND hwnd, HINSTANCE hInstance);
LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

} /* namespace input */
} /* namespace win32 */
