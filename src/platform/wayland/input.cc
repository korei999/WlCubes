#include "../../controls.hh"
#include "../../frame.hh"
#include "WlClient.hh"
#include "logs.hh"

namespace wayland
{
namespace input
{

void
keyboardKeymapHandler([[maybe_unused]] void* data,
                      [[maybe_unused]] wl_keyboard* keyboard,
                      [[maybe_unused]] u32 format,
                      [[maybe_unused]] s32 fd,
                      [[maybe_unused]] u32 size)
{
    //
}

void
keyboardEnterHandler([[maybe_unused]] void* data,
                     [[maybe_unused]] wl_keyboard* keyboard,
                     [[maybe_unused]] u32 serial,
                     [[maybe_unused]] wl_surface* surface,
                     [[maybe_unused]] wl_array* keys)
{
    LOG_OK("keyboardEnterHandler\n");

    auto app = (WlClient*)(data);

    if (app->_bRelativeMode) app->enableRelativeMode();
}

void
keyboardLeaveHandler([[maybe_unused]] void* data,
                     [[maybe_unused]] wl_keyboard* keyboard,
                     [[maybe_unused]] u32 serial,
                     [[maybe_unused]] wl_surface* surface)
{
    LOG_OK("keyboardLeaveHandler\n");

    auto app = (WlClient*)(data);

    /* prevent keys getting stuck after leaving surface */
    for (auto& key : controls::pressedKeys) key = 0;

    if (app->_bRelativeMode) app->disableRelativeMode();
}

void
keyboardKeyHandler([[maybe_unused]] void* data,
                   [[maybe_unused]] wl_keyboard* keyboard,
                   [[maybe_unused]] u32 serial,
                   [[maybe_unused]] u32 time,
                   [[maybe_unused]] u32 key,
                   [[maybe_unused]] u32 keyState)
{
    auto app = (WlClient*)(data);

#ifdef DEBUG
    if (key >= adt::size(controls::pressedKeys))
    {
        LOG_WARN("key '%u' is too big?\n", key);
        return;
    }
#endif

    controls::pressedKeys[key] = keyState;
    controls::procKeysOnce(app, key, keyState);
}

void
keyboardModifiersHandler([[maybe_unused]] void* data,
                         [[maybe_unused]] wl_keyboard* keyboard,
                         [[maybe_unused]] u32 serial,
                         [[maybe_unused]] u32 modsDepressed,
                         [[maybe_unused]] u32 modsLatched,
                         [[maybe_unused]] u32 modsLocked,
                         [[maybe_unused]] u32 group)
{
    //
}

void
keyboardRepeatInfoHandler([[maybe_unused]] void* data,
                          [[maybe_unused]] wl_keyboard* wl_keyboard,
                          [[maybe_unused]] s32 rate,
                          [[maybe_unused]] s32 delay)
{
    //
}


void
pointerEnterHandler([[maybe_unused]] void* data,
                    [[maybe_unused]] wl_pointer* pointer,
                    [[maybe_unused]] u32 serial,
                    [[maybe_unused]] wl_surface* surface,
                    [[maybe_unused]] wl_fixed_t surfaceX,
                    [[maybe_unused]] wl_fixed_t surfaceY)
{
    LOG_OK("pointerEnterHandler\n");

    auto app = (WlClient*)(data);
    app->_pointerSerial = serial;

    if (app->_bRelativeMode)
        wl_pointer_set_cursor(pointer, serial, nullptr, 0, 0);
    else
        wl_pointer_set_cursor(pointer,
                              serial,
                              app->_cursorSurface,
                              app->_cursorImage->hotspot_x,
                              app->_cursorImage->hotspot_y);
}

void
pointerLeaveHandler([[maybe_unused]] void* data,
                    [[maybe_unused]] wl_pointer* pointer,
                    [[maybe_unused]] u32 serial,
                    [[maybe_unused]] wl_surface* surface)
{
    LOG_OK("pointerLeaveHandler\n");
}

void
pointerMotionHandler([[maybe_unused]] void* data,
                     [[maybe_unused]] wl_pointer* pointer,
                     [[maybe_unused]] u32 time,
                     [[maybe_unused]] wl_fixed_t surfaceX,
                     [[maybe_unused]] wl_fixed_t surfaceY)
{
    frame::player._mouse.absX = wl_fixed_to_double(surfaceX);
    frame::player._mouse.absY = wl_fixed_to_double(surfaceY);
}

void
pointerButtonHandler([[maybe_unused]] void* data,
                     [[maybe_unused]] wl_pointer* pointer,
                     [[maybe_unused]] u32 serial,
                     [[maybe_unused]] u32 time,
                     [[maybe_unused]] u32 button,
                     [[maybe_unused]] u32 buttonState)
{
    frame::player._mouse.button = button;
    frame::player._mouse.state = buttonState;
}

void
pointerAxisHandler([[maybe_unused]] void* data,
                   [[maybe_unused]] wl_pointer* pointer,
                   [[maybe_unused]] u32 time,
                   [[maybe_unused]] u32 axis,
                   [[maybe_unused]] wl_fixed_t value)
{
    //
}

void
relativePointerMotionHandler([[maybe_unused]] void *data,
				             [[maybe_unused]] zwp_relative_pointer_v1 *zwp_relative_pointer_v1,
				             [[maybe_unused]] u32 utime_hi,
				             [[maybe_unused]] u32 utime_lo,
				             [[maybe_unused]] wl_fixed_t dx,
				             [[maybe_unused]] wl_fixed_t dy,
				             [[maybe_unused]] wl_fixed_t dxUnaccel,
				             [[maybe_unused]] wl_fixed_t dyUnaccel)
{
    frame::player._mouse.relX += wl_fixed_to_int(dxUnaccel);
    frame::player._mouse.relY += wl_fixed_to_int(dyUnaccel);
}

} /* namespace input */
} /* namespace wayland */
