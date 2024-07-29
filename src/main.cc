#include "ListAllocator.hh"
#include "frame.hh"

#ifdef __linux__
    #include "platform/wayland/WlClient.hh"

int
main()
{
    adt::ListAllocator allocator;

    wayland::WlClient app(&allocator, "WlCubes");
    frame::run(&app);

    allocator.freeAll();
}

#elif _WIN32
    #include "platform/windows/windows.hh"

int WINAPI
WinMain([[maybe_unused]] HINSTANCE instance,
        [[maybe_unused]] HINSTANCE previnstance,
        [[maybe_unused]] LPSTR cmdline,
        [[maybe_unused]] int cmdshow)
{
    adt::ListAllocator allocator;

    win32::Window app(&allocator, "wl-cube", instance);
    frame::run(&app);

    allocator.freeAll();
}

    #ifdef DEBUG
int
main()
{
    return WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOWNORMAL);
}
    #endif

#endif
