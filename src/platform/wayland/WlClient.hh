#pragma once

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <wayland-client-protocol.h>
#include <wayland-cursor.h>
#include <wayland-egl-core.h>

#include "Allocator.hh"
#include "ultratypes.h"
#include "wayland-protocols/pointer-constraints-unstable-v1.h"
#include "wayland-protocols/relative-pointer-unstable-v1.h"
#include "wayland-protocols/xdg-shell.h"

#include "../../App.hh"

namespace wayland
{

struct WlClient : App
{
    wl_display* _display {};
    wl_registry* _registry {};

    wl_surface* _surface {};
    xdg_surface* _xdgSurface {};
    xdg_toplevel* _xdgToplevel {};
    wl_output* _output {};

    wl_egl_window* _eglWindow {};
    EGLDisplay _eglDisplay {};
    EGLContext _eglContext {};
    EGLSurface _eglSurface {};

    wl_seat* _seat {};
    wl_shm* _shm {};
    wl_compositor* _compositor {};
    xdg_wm_base* _xdgWmBase {};
    [[maybe_unused]] u32 _xdgConfigureSerial = 0;

    wl_pointer* _pointer {};
    wl_surface* _cursorSurface {};
    wl_cursor_image* _cursorImage {};
    wl_cursor_theme* _cursorTheme {};

    u32 _pointerSerial = 0;
    zwp_pointer_constraints_v1* _pointerConstraints {};
    zwp_locked_pointer_v1* _lockedPointer {};
    zwp_confined_pointer_v1* _confinedPointer {};
    zwp_relative_pointer_v1* _relativePointer {};
    zwp_relative_pointer_manager_v1* _relativePointerManager {};

    wl_keyboard* _keyboard {};

    WlClient(adt::Allocator* pAlloc, adt::String name);
    virtual ~WlClient() override;

    virtual void init() override;
    virtual void disableRelativeMode() override;
    virtual void enableRelativeMode() override;
    virtual void togglePointerRelativeMode() override;
    virtual void toggleFullscreen() override;
    virtual void setCursorImage(adt::String cursorType) override;
    virtual void setFullscreen() override;
    virtual void unsetFullscreen() override;
    virtual void bindGlContext() override;
    virtual void unbindGlContext() override;
    virtual void setSwapInterval(int interval) override;
    virtual void toggleVSync() override;
    virtual void swapBuffers() override;
    virtual void procEvents() override;
    virtual void showWindow() override;
};

} /* namespace wayland */
