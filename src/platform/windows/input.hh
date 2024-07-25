#pragma once
#include "windows.hh"

namespace win32
{
namespace input
{

void registerRawMouseDevice(Window* self, bool on);
void registerRawKBDevice(Window* self, bool on);
LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

} /* namespace input */
} /* namespace win32 */
