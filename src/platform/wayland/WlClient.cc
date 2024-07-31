#define _POSIX_C_SOURCE 199309L

#include "ArenaAllocator.hh"
#include "Array.hh"
#include "WlClient.hh"
#include "input.hh"
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
    xdg_wm_base_pong(app->_xdgWmBase, serial);
}

static void
seatCapabilitiesHandler([[maybe_unused]] void* data,
                        [[maybe_unused]] wl_seat* seat,
                        [[maybe_unused]] u32 capabilities)
{
    auto app = (WlClient*)data;

    if (capabilities & WL_SEAT_CAPABILITY_POINTER)
    {
        app->_pointer = wl_seat_get_pointer(app->_seat);
        app->_cursorTheme = wl_cursor_theme_load(nullptr, 24, app->_shm);
        wl_pointer_add_listener(app->_pointer, &pointerListener, app);
        LOG_OK("pointer works.\n");
    }
    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD)
    {
        app->_keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(app->_keyboard, &keyboardListener, app);
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
    app->_bConfigured = true;
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
        if (width != app->_wWidth || height != app->_wHeight)
        {
            wl_egl_window_resize(app->_eglWindow, width, height, 0, 0);
            app->_wWidth = width;
            app->_wHeight = height;
        }
    }
}

static void
xdgToplevelCloseHandler([[maybe_unused]] void* data,
		                [[maybe_unused]] xdg_toplevel* xdgToplevel)
{
    auto app = (WlClient*)data;
    app->_bRunning = false;
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
        app->_seat = (wl_seat*)wl_registry_bind(registry, name, &wl_seat_interface, 1);
        wl_seat_add_listener(app->_seat, &seatListener, app);
    }
    else if (strcmp(interface, wl_compositor_interface.name) == 0)
    {
        app->_compositor = (wl_compositor*)wl_registry_bind(registry, name, &wl_compositor_interface, version);
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
        app->_xdgWmBase = (xdg_wm_base*)wl_registry_bind(registry, name, &xdg_wm_base_interface, version);
        xdg_wm_base_add_listener(app->_xdgWmBase, &xdgWmBaseListener, app);
    }
    else if (strcmp(interface, zwp_pointer_constraints_v1_interface.name) == 0)
    {
        app->_pointerConstraints = (zwp_pointer_constraints_v1*)wl_registry_bind(registry, name, &zwp_pointer_constraints_v1_interface, version);
    }
    else if (strcmp(interface, zwp_relative_pointer_manager_v1_interface.name) == 0)
    {
        app->_relativePointerManager = (zwp_relative_pointer_manager_v1*)wl_registry_bind(registry, name, &zwp_relative_pointer_manager_v1_interface, version);
    }
    else if (strcmp(interface, wl_shm_interface.name) == 0)
    {
        app->_shm = (wl_shm*)wl_registry_bind(registry, name, &wl_shm_interface, version);
    }
    else if (strcmp(interface, wl_output_interface.name) == 0)
    {
        app->_output = (wl_output*)wl_registry_bind(registry, name, &wl_output_interface, version);
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
{
    _pAlloc = _allocator;
    _svName = name;
    init();
}

WlClient::~WlClient()
{
    LOG_OK("cleanup ...\n");
    if (_bRelativeMode)
        disableRelativeMode();
    if (_pointer)
        wl_pointer_destroy(_pointer);
    if (_cursorTheme)
        wl_cursor_theme_destroy(_cursorTheme);
    if (_keyboard)
        wl_keyboard_destroy(_keyboard);
    if (_xdgSurface)
        xdg_surface_destroy(_xdgSurface);
    if (_xdgToplevel)
        xdg_toplevel_destroy(_xdgToplevel);
    if (_surface)
        wl_surface_destroy(_surface);
    if (_shm)
        wl_shm_destroy(_shm);
    if (_xdgWmBase)
        xdg_wm_base_destroy(_xdgWmBase);
    if (_pointerConstraints)
        zwp_pointer_constraints_v1_destroy(_pointerConstraints);
    if (_relativePointerManager)
        zwp_relative_pointer_manager_v1_destroy(_relativePointerManager);
    if (_cursorSurface)
        wl_surface_destroy(_cursorSurface);
    if (_seat)
        wl_seat_destroy(_seat);
    if (_output)
        wl_output_destroy(_output);
    if (_compositor)
        wl_compositor_destroy(_compositor);
    if (_registry)
        wl_registry_destroy(_registry);
}

void
WlClient::init()
{
    adt::ArenaAllocator arena(adt::SIZE_8K);

    if ((_display = wl_display_connect(nullptr)))
        LOG_OK("wayland display connected\n");
    else
        LOG_OK("error connecting wayland display\n");

    _registry = wl_display_get_registry(_display);
    wl_registry_add_listener(_registry, &registryListener, this);
    wl_display_dispatch(_display);
    wl_display_roundtrip(_display);

    if (_compositor == nullptr || _xdgWmBase == nullptr)
        LOG_FATAL("no wl_shm, wl_compositor or xdg_wm_base support\n");

    EGLD( _eglDisplay = eglGetDisplay(reinterpret_cast<EGLNativeDisplayType>(_display)) );
    if (_eglDisplay == EGL_NO_DISPLAY)
        LOG_FATAL("failed to create EGL display\n");

    EGLint major, minor;
    if (!eglInitialize(_eglDisplay, &major, &minor))
        LOG_FATAL("failed to initialize EGL\n");
    EGLD();

    LOG_OK("egl: major: %d, minor: %d\n", major, minor);

    EGLint count;
    EGLD(eglGetConfigs(_eglDisplay, nullptr, 0, &count));

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
    EGLD( eglChooseConfig(_eglDisplay, configAttribs, configs.data(), count, &n) );
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

    EGLD( _eglContext = eglCreateContext(_eglDisplay, eglConfig, EGL_NO_CONTEXT, contextAttribs) );

    _surface = wl_compositor_create_surface(_compositor);
    _xdgSurface = xdg_wm_base_get_xdg_surface(_xdgWmBase, _surface);
    _xdgToplevel = xdg_surface_get_toplevel(_xdgSurface);

    xdg_toplevel_set_title(_xdgToplevel, _svName.data());
    xdg_toplevel_set_app_id(_xdgToplevel, _svName.data());

    xdg_surface_add_listener(_xdgSurface, &xdgSurfaceListener, this);
    xdg_toplevel_add_listener(_xdgToplevel, &xdgToplevelListener, this);

    _eglWindow = wl_egl_window_create(_surface, _wWidth, _wHeight);
    EGLD( _eglSurface = eglCreateWindowSurface(_eglDisplay, eglConfig, reinterpret_cast<EGLNativeWindowType>(_eglWindow), nullptr) );

    wl_surface_commit(_surface);
    wl_display_roundtrip(_display);

    arena.freeAll();
}

void
WlClient::enableRelativeMode()
{
    wl_pointer_set_cursor(_pointer, _pointerSerial, nullptr, 0, 0);
    if (_cursorSurface) wl_surface_destroy(_cursorSurface);
    _lockedPointer = zwp_pointer_constraints_v1_lock_pointer(_pointerConstraints,
                                                                  _surface,
                                                                  _pointer,
                                                                  nullptr,
                                                                  ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_ONESHOT);
    _relativePointer = zwp_relative_pointer_manager_v1_get_relative_pointer(_relativePointerManager, _pointer);
    zwp_relative_pointer_v1_add_listener(_relativePointer, &relativePointerListener, this);
}

void
WlClient::disableRelativeMode()
{
    zwp_locked_pointer_v1_destroy(_lockedPointer);
    zwp_relative_pointer_v1_destroy(_relativePointer);

    setCursorImage("default");
}

void
WlClient::setCursorImage(adt::String cursorType)
{
    wl_cursor* cursor = wl_cursor_theme_get_cursor(_cursorTheme, cursorType.data());
    if (!cursor)
    {
        LOG_WARN("failed to set cursor to '%s', falling back to 'default'\n", cursorType.data());
        cursor = wl_cursor_theme_get_cursor(_cursorTheme, "default");
    }

    if (cursor)
    {
        _cursorImage = cursor->images[0];
        wl_buffer* cursorBuffer = wl_cursor_image_get_buffer(_cursorImage);

        _cursorSurface = wl_compositor_create_surface(_compositor);
        wl_pointer_set_cursor(_pointer, _pointerSerial, _cursorSurface, 0, 0);
        wl_surface_attach(_cursorSurface, cursorBuffer, 0, 0);
        wl_surface_commit(_cursorSurface);

        wl_pointer_set_cursor(_pointer,
                              _pointerSerial,
                              _cursorSurface,
                              _cursorImage->hotspot_x,
                              _cursorImage->hotspot_y);
    }
}

void 
WlClient::setFullscreen()
{
    xdg_toplevel_set_fullscreen(_xdgToplevel, _output);
}

void
WlClient::unsetFullscreen()
{
    xdg_toplevel_unset_fullscreen(_xdgToplevel);
}

void
WlClient::togglePointerRelativeMode()
{
    _bRelativeMode = !_bRelativeMode;
    _bRelativeMode ? enableRelativeMode() : disableRelativeMode();
    LOG_OK("relative mode: %d\n", _bRelativeMode);
}

void
WlClient::toggleFullscreen()
{
    _bFullscreen = !_bFullscreen;
    _bFullscreen ? setFullscreen() : unsetFullscreen();
}

void 
WlClient::bindGlContext()
{
    EGLD ( eglMakeCurrent(_eglDisplay, _eglSurface, _eglSurface, _eglContext) );
}

void 
WlClient::unbindGlContext()
{
    EGLD( eglMakeCurrent(_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) );
}

void
WlClient::setSwapInterval(int interval)
{
    _swapInterval = interval;
    EGLD( eglSwapInterval(_eglDisplay, interval) );
}

void
WlClient::toggleVSync()
{
    _swapInterval = !_swapInterval;
    EGLD( eglSwapInterval(_eglDisplay, _swapInterval) );
    LOG_OK("swapInterval: %d\n", _swapInterval);
}

void
WlClient::swapBuffers()
{
    EGLD( eglSwapBuffers(_eglDisplay, _eglSurface) );
    if (wl_display_dispatch(_display) == -1)
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
