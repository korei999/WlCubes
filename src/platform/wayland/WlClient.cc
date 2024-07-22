#define _POSIX_C_SOURCE 199309L

#include "Array.hh"
#include "Arena.hh"
#include "input.hh"
#include "WlClient.hh"
#include "logs.hh"

namespace wayland
{

#include <string.h>

EGLint eglLastErrorCode = EGL_SUCCESS;

#ifdef DEBUG
#    define EGLD(C)                                                                                                    \
        {                                                                                                              \
            C;                                                                                                         \
            if ((eglLastErrorCode = eglGetError()) != EGL_SUCCESS)                                                     \
                LOG_FATAL("eglLastErrorCode: %#x\n", eglLastErrorCode);                                                \
        }
#else
#    define EGLD(C) C
#endif

static const zwp_relative_pointer_v1_listener relativePointerListener {
	.relative_motion = input::relativePointerMotionHandler
};

static const wl_pointer_listener pointerListener {
    .enter = input::pointerEnterHandler,
    .leave = input::pointerLeaveHandler,
    .motion = input::pointerMotionHandler,
    .button = input::pointerButtonHandler,
    .axis = input::pointerAxisHandler,
    .frame {},
    .axis_source {},
    .axis_stop {},
    .axis_discrete {},
    .axis_value120 {},
    .axis_relative_direction {}
};

static const wl_keyboard_listener keyboardListener {
    .keymap = input::keyboardKeymapHandler,
    .enter = input::keyboardEnterHandler,
    .leave = input::keyboardLeaveHandler,
    .key = input::keyboardKeyHandler,
    .modifiers = input::keyboardModifiersHandler,
    .repeat_info = input::keyboardRepeatInfoHandler
};

/* mutter compositor will complain if we do not pong */
static void
xdgWmBasePing([[maybe_unused]] void* data,
              [[maybe_unused]] xdg_wm_base* xdgWmBase,
              [[maybe_unused]] u32 serial)
{
    auto app = (WlClient*)data;
    xdg_wm_base_pong(app->xdgWmBase, serial);
}

static void
seatCapabilitiesHandler([[maybe_unused]] void* data,
                        [[maybe_unused]] wl_seat* seat,
                        [[maybe_unused]] u32 capabilities)
{
    auto app = (WlClient*)data;

    if (capabilities & WL_SEAT_CAPABILITY_POINTER)
    {
        app->pointer = wl_seat_get_pointer(app->seat);
        app->cursorTheme = wl_cursor_theme_load(nullptr, 24, app->shm);
        wl_pointer_add_listener(app->pointer, &pointerListener, app);
        LOG_OK("pointer works.\n");
    }
    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD)
    {
        app->keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(app->keyboard, &keyboardListener, app);
        LOG_OK("keyboard works.\n");
    }
    if (capabilities & WL_SEAT_CAPABILITY_TOUCH)
    {
        //
        LOG_OK("touch works.\n");
    }
}

static const wl_seat_listener seatListener {
    .capabilities = seatCapabilitiesHandler,
	.name = {}
};

static void
xdgSurfaceConfigureHandler([[maybe_unused]] void* data,
                           [[maybe_unused]] xdg_surface* xdgSurface,
                           [[maybe_unused]] u32 serial)
{
    auto app = (WlClient*)data;
    xdg_surface_ack_configure(xdgSurface, serial);
    app->bConfigured = true;
}

static void
xdgToplevelConfigureHandler([[maybe_unused]] void* data,
                            [[maybe_unused]] xdg_toplevel* xdgToplevel,
                            [[maybe_unused]] s32 width,
                            [[maybe_unused]] s32 height,
                            [[maybe_unused]] wl_array* states)
{
    auto app = (WlClient*)data;

    if (width > 0 && height > 0)
    {
        if (width != app->wWidth || height != app->wHeight)
        {
            wl_egl_window_resize(app->eglWindow, width, height, 0, 0);
            app->wWidth = width;
            app->wHeight = height;
        }
    }
}

static void
xdgToplevelCloseHandler([[maybe_unused]] void* data,
		                [[maybe_unused]] xdg_toplevel* xdgToplevel)
{
    auto app = (WlClient*)data;
    app->bRunning = false;
    LOG_OK("closing...\n");
}

static void
xdgToplevelConfigureBounds([[maybe_unused]] void* data,
                           [[maybe_unused]] xdg_toplevel* xdgToplevel,
                           [[maybe_unused]] s32 width,
                           [[maybe_unused]] s32 height)
{
    //
}

static const xdg_surface_listener xdgSurfaceListener {
    .configure = xdgSurfaceConfigureHandler
};

static const xdg_wm_base_listener xdgWmBaseListener {
    .ping = xdgWmBasePing
};


static void
xdgToplevelWmCapabilities([[maybe_unused]] void* data,
                          [[maybe_unused]] xdg_toplevel* xdgToplevel,
                          [[maybe_unused]] wl_array* capabilities)
{
    //
}

static const xdg_toplevel_listener xdgToplevelListener {
    .configure = xdgToplevelConfigureHandler,
    .close = xdgToplevelCloseHandler,
    .configure_bounds = xdgToplevelConfigureBounds,
    .wm_capabilities = xdgToplevelWmCapabilities
};

static void
registryGlobalHandler([[maybe_unused]] void* data,
                      [[maybe_unused]] wl_registry* registry,
                      [[maybe_unused]] u32 name,
                      [[maybe_unused]] const char* interface,
                      [[maybe_unused]] u32 version)
{
    LOG_OK("interface: '%s', version: %u, name: %u\n", interface, version, name);
    auto app = (WlClient*)data;

    if (strcmp(interface, wl_seat_interface.name) == 0)
    {
        app->seat = (wl_seat*)wl_registry_bind(registry, name, &wl_seat_interface, 1);
        wl_seat_add_listener(app->seat, &seatListener, app);
    }
    else if (strcmp(interface, wl_compositor_interface.name) == 0)
    {
        app->compositor = (wl_compositor*)wl_registry_bind(registry, name, &wl_compositor_interface, version);
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
        app->xdgWmBase = (xdg_wm_base*)wl_registry_bind(registry, name, &xdg_wm_base_interface, version);
        xdg_wm_base_add_listener(app->xdgWmBase, &xdgWmBaseListener, app);
    }
    else if (strcmp(interface, zwp_pointer_constraints_v1_interface.name) == 0)
    {
        app->pointerConstraints = (zwp_pointer_constraints_v1*)wl_registry_bind(registry, name, &zwp_pointer_constraints_v1_interface, version);
    }
    else if (strcmp(interface, zwp_relative_pointer_manager_v1_interface.name) == 0)
    {
        app->relativePointerManager = (zwp_relative_pointer_manager_v1*)wl_registry_bind(registry, name, &zwp_relative_pointer_manager_v1_interface, version);
    }
    else if (strcmp(interface, wl_shm_interface.name) == 0)
    {
        app->shm = (wl_shm*)wl_registry_bind(registry, name, &wl_shm_interface, version);
    }
    else if (strcmp(interface, wl_output_interface.name) == 0)
    {
        app->output = (wl_output*)wl_registry_bind(registry, name, &wl_output_interface, version);
    }
}

static void
registryGlobalRemoveHandler([[maybe_unused]] void* data,
                            [[maybe_unused]] wl_registry* wlRegistry,
                            [[maybe_unused]] u32 name)
{
    /* can be empty */
}

static const wl_registry_listener registryListener {
    .global = registryGlobalHandler,
    .global_remove = registryGlobalRemoveHandler
};

WlClient::WlClient(adt::Allocator* _allocator, adt::String name)
    : allocator(_allocator)
{
    this->svName = name;
    this->init();
}

WlClient::~WlClient()
{
    LOG_OK("cleanup ...\n");
    if (bRelativeMode)
        disableRelativeMode();
    if (pointer)
        wl_pointer_destroy(pointer);
    if (cursorTheme)
        wl_cursor_theme_destroy(cursorTheme);
    if (keyboard)
        wl_keyboard_destroy(keyboard);
    if (xdgSurface)
        xdg_surface_destroy(xdgSurface);
    if (xdgToplevel)
        xdg_toplevel_destroy(xdgToplevel);
    if (surface)
        wl_surface_destroy(surface);
    if (shm)
        wl_shm_destroy(shm);
    if (xdgWmBase)
        xdg_wm_base_destroy(xdgWmBase);
    if (pointerConstraints)
        zwp_pointer_constraints_v1_destroy(pointerConstraints);
    if (relativePointerManager)
        zwp_relative_pointer_manager_v1_destroy(relativePointerManager);
    if (cursorSurface)
        wl_surface_destroy(cursorSurface);
    if (seat)
        wl_seat_destroy(seat);
    if (output)
        wl_output_destroy(output);
    if (compositor)
        wl_compositor_destroy(compositor);
    if (registry)
        wl_registry_destroy(registry);
}

void
WlClient::init()
{
    adt::Arena arena(adt::SIZE_8K);

    if ((this->display = wl_display_connect(nullptr)))
        LOG_OK("wayland display connected\n");
    else
        LOG_OK("error connecting wayland display\n");

    this->registry = wl_display_get_registry(this->display);
    wl_registry_add_listener(this->registry, &registryListener, this);
    wl_display_dispatch(this->display);
    wl_display_roundtrip(this->display);

    if (this->compositor == nullptr || this->xdgWmBase == nullptr)
        LOG_FATAL("no wl_shm, wl_compositor or xdg_wm_base support\n");

    EGLD( this->eglDisplay = eglGetDisplay(reinterpret_cast<EGLNativeDisplayType>(this->display)) );
    if (this->eglDisplay == EGL_NO_DISPLAY)
        LOG_FATAL("failed to create EGL display\n");

    EGLint major, minor;
    if (!eglInitialize(this->eglDisplay, &major, &minor))
        LOG_FATAL("failed to initialize EGL\n");
    EGLD();

    LOG_OK("egl: major: %d, minor: %d\n", major, minor);

    EGLint count;
    EGLD(eglGetConfigs(this->eglDisplay, nullptr, 0, &count));

    EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        // EGL_ALPHA_SIZE, 8, /* KDE makes window transparent even in fullscreen */
        EGL_DEPTH_SIZE, 24,
        EGL_STENCIL_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
        // EGL_SAMPLE_BUFFERS, 1,
        // EGL_SAMPLES, 4,
        EGL_NONE
    };

    EGLint n = 0;
    adt::Array<EGLConfig> configs(&arena, count);
    EGLD( eglChooseConfig(this->eglDisplay, configAttribs, configs.data(), count, &n) );
    if (n == 0)
        LOG_FATAL("Failed to choose an EGL config\n");

    EGLConfig eglConfig = configs[0];

    EGLint contextAttribs[] {
        EGL_CONTEXT_CLIENT_VERSION, 3,
#ifdef DEBUG
        EGL_CONTEXT_OPENGL_DEBUG, EGL_TRUE,
#endif
        EGL_NONE,
    };

    EGLD( eglContext = eglCreateContext(this->eglDisplay, eglConfig, EGL_NO_CONTEXT, contextAttribs) );

    surface = wl_compositor_create_surface(this->compositor);
    xdgSurface = xdg_wm_base_get_xdg_surface(this->xdgWmBase, this->surface);
    xdgToplevel = xdg_surface_get_toplevel(this->xdgSurface);

    xdg_toplevel_set_title(this->xdgToplevel, this->svName.data());
    xdg_toplevel_set_app_id(this->xdgToplevel, this->svName.data());

    xdg_surface_add_listener(this->xdgSurface, &xdgSurfaceListener, this);
    xdg_toplevel_add_listener(this->xdgToplevel, &xdgToplevelListener, this);

    eglWindow = wl_egl_window_create(this->surface, this->wWidth, this->wHeight);
    EGLD( eglSurface = eglCreateWindowSurface(this->eglDisplay, eglConfig, reinterpret_cast<EGLNativeWindowType>(this->eglWindow), nullptr) );

    wl_surface_commit(this->surface);
    wl_display_roundtrip(this->display);

    arena.freeAll();
}

void
WlClient::enableRelativeMode()
{
    wl_pointer_set_cursor(this->pointer, this->pointerSerial, nullptr, 0, 0);
    if (this->cursorSurface) wl_surface_destroy(this->cursorSurface);
    this->lockedPointer = zwp_pointer_constraints_v1_lock_pointer(this->pointerConstraints,
                                                                  this->surface,
                                                                  this->pointer,
                                                                  nullptr,
                                                                  ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_ONESHOT);
    relativePointer = zwp_relative_pointer_manager_v1_get_relative_pointer(this->relativePointerManager, this->pointer);
    zwp_relative_pointer_v1_add_listener(this->relativePointer, &relativePointerListener, this);
}

void
WlClient::disableRelativeMode()
{
    zwp_locked_pointer_v1_destroy(this->lockedPointer);
    zwp_relative_pointer_v1_destroy(this->relativePointer);

    this->setCursorImage("default");
}

void
WlClient::setCursorImage(adt::String cursorType)
{
    wl_cursor* cursor = wl_cursor_theme_get_cursor(this->cursorTheme, cursorType.data());
    if (!cursor)
    {
        LOG_WARN("failed to set cursor to '%s', falling back to 'default'\n", cursorType.data());
        cursor = wl_cursor_theme_get_cursor(this->cursorTheme, "default");
    }

    if (cursor)
    {
        cursorImage = cursor->images[0];
        wl_buffer* cursorBuffer = wl_cursor_image_get_buffer(this->cursorImage);

        cursorSurface = wl_compositor_create_surface(this->compositor);
        wl_pointer_set_cursor(this->pointer, this->pointerSerial, this->cursorSurface, 0, 0);
        wl_surface_attach(this->cursorSurface, cursorBuffer, 0, 0);
        wl_surface_commit(this->cursorSurface);

        wl_pointer_set_cursor(this->pointer,
                              this->pointerSerial,
                              this->cursorSurface,
                              this->cursorImage->hotspot_x,
                              this->cursorImage->hotspot_y);
    }
}

void 
WlClient::setFullscreen()
{
    xdg_toplevel_set_fullscreen(this->xdgToplevel, this->output);
}

void
WlClient::unsetFullscreen()
{
    xdg_toplevel_unset_fullscreen(this->xdgToplevel);
}

void
WlClient::togglePointerRelativeMode()
{
    this->bRelativeMode = !this->bRelativeMode;
    this->bRelativeMode ? this->enableRelativeMode() : this->disableRelativeMode();
    LOG_OK("relative mode: %d\n", this->bRelativeMode);
}

void
WlClient::toggleFullscreen()
{
    this->bFullscreen = !this->bFullscreen;
    this->bFullscreen ? this->setFullscreen() : this->unsetFullscreen();
}

void 
WlClient::bindGlContext()
{
    EGLD ( eglMakeCurrent(this->eglDisplay, this->eglSurface, this->eglSurface, this->eglContext) );
}

void 
WlClient::unbindGlContext()
{
    EGLD( eglMakeCurrent(this->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) );
}

void
WlClient::setSwapInterval(int interval)
{
    this->swapInterval = interval;
    EGLD( eglSwapInterval(this->eglDisplay, interval) );
}

void
WlClient::toggleVSync()
{
    this->swapInterval = !this->swapInterval;
    EGLD( eglSwapInterval(this->eglDisplay, this->swapInterval) );
    LOG_OK("swapInterval: %d\n", this->swapInterval);
}

void
WlClient::swapBuffers()
{
    EGLD( eglSwapBuffers(this->eglDisplay, this->eglSurface) );
    if (wl_display_dispatch(this->display) == -1)
        LOG_FATAL("wl_display_dispatch error\n");
}

void 
WlClient::procEvents()
{
    //
}

void
WlClient::showWindow()
{
    //
}

} /* namespace wayland */
