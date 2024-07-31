#pragma once

#include "String.hh"

struct App
{
    adt::Allocator* _pAlloc;
    adt::String _svName;
    int _wWidth = 1920;
    int _wHeight = 1080;
    bool _bRunning = false;
    bool _bConfigured = false;
    int _swapInterval = 1;
    bool bPaused = false;
    bool _bRelativeMode = false;
    bool _bFullscreen = false;

    virtual ~App() = default;

    virtual void init() = 0;
    virtual void disableRelativeMode() = 0;
    virtual void enableRelativeMode() = 0;
    virtual void togglePointerRelativeMode() = 0;
    virtual void toggleFullscreen() = 0;
    virtual void setCursorImage(adt::String cursorType) = 0;
    virtual void setFullscreen() = 0;
    virtual void unsetFullscreen() = 0;
    virtual void bindGlContext() = 0;
    virtual void unbindGlContext() = 0;
    virtual void setSwapInterval(int interval) = 0;
    virtual void toggleVSync() = 0;
    virtual void swapBuffers() = 0;
    virtual void procEvents() = 0;
    virtual void showWindow() = 0;
};
