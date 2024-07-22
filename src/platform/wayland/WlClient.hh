#pragma once

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <wayland-client-protocol.h>
#include <wayland-cursor.h>
#include <wayland-egl-core.h>

#include "Arena.hh"
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
    adt::Allocator* allocator;

    wl_display* display {};
    wl_registry* registry {};

    wl_surface* surface {};
    xdg_surface* xdgSurface {};
    xdg_toplevel* xdgToplevel {};
    wl_output* output {};

    wl_egl_window *eglWindow {};
    EGLDisplay eglDisplay {};
    EGLContext eglContext {};
    EGLSurface eglSurface {};

    wl_seat* seat {};
    wl_shm* shm {};
    wl_compositor* compositor {};
    xdg_wm_base* xdgWmBase {};
    [[maybe_unused]] u32 xdgConfigureSerial = 0;

    wl_pointer* pointer {};
    wl_surface* cursorSurface {};
    wl_cursor_image* cursorImage {};
    wl_cursor_theme* cursorTheme {};

    u32 pointerSerial = 0;
    zwp_pointer_constraints_v1* pointerConstraints {};
    zwp_locked_pointer_v1* lockedPointer {};
    zwp_confined_pointer_v1* confinedPointer {};
    zwp_relative_pointer_v1* relativePointer {};
    zwp_relative_pointer_manager_v1* relativePointerManager {};

    wl_keyboard* keyboard {};

    WlClient(adt::Allocator* _allocator, adt::String name);
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
