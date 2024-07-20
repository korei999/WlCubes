#include <stdio.h>

#ifdef __linux__
    #include "platform/wayland/WlClient.hh"
#endif

#include "MapAllocator.hh"
#include "frame.hh"

int
main()
{
    adt::MapAllocator allocator;

    wayland::WlClient app(&allocator, "WlCubes");
    frame::run(&app);

    allocator.freeAll();
}
