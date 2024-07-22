#pragma once
#include "../../App.hh"

#include <windef.h>

namespace win32
{

struct Window : App
{
    HINSTANCE hInstance;
    HWND hWindow;
    HDC hDeviceContext;
    HGLRC hGlContext;
    WNDCLASSEXW windowClass;

    Window(adt::Allocator* p, adt::String name, HINSTANCE _instance);
    virtual ~Window() override;

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

};
