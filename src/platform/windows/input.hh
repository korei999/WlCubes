#pragma once
#include "windows.hh"

namespace win32
{
namespace input
{

void initRawDevices(Window* self);
LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

} /* namespace input */
} /* namespace win32 */
