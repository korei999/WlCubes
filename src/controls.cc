#include <math.h>

#include "controls.hh"
#include "frame.hh"
#include "logs.hh"

#ifdef __linux__
    #include <linux/input-event-codes.h>
#elif _WIN32
    #undef near
    #undef far
#endif

namespace controls
{

bool pressedKeys[300] {};

void
PlayerControls::procMouse()
{
    f64 offsetX = (_mouse.relX - _mouse.prevRelX) * _mouse.sens;
    f64 offsetY = (_mouse.prevRelY - _mouse.relY) * _mouse.sens;

    _mouse.prevRelX = _mouse.relX;
    _mouse.prevRelY = _mouse.relY;

    _mouse.yaw += offsetX;
    _mouse.pitch += offsetY;

    if (_mouse.pitch > 89.9)
        _mouse.pitch = 89.9;
    if (_mouse.pitch < -89.9)
        _mouse.pitch = -89.9;

    _front = v3Norm({
        f32(cos(toRad(_mouse.yaw)) * cos(toRad(_mouse.pitch))),
        f32(sin(toRad(_mouse.pitch))),
        f32(sin(toRad(_mouse.yaw)) * cos(toRad(_mouse.pitch)))
    });

    _right = v3Norm(v3Cross(_front, _up));
}

void
procKeysOnce(App* app, u32 key, u32 pressed)
{
    switch (key)
    {
        case KEY_P:
        case KEY_GRAVE:
            if (pressed)
            {
                app->bPaused = !app->bPaused;
                if (app->bPaused) LOG_WARN("paused: %d\n", app->bPaused);
            }
            break;

        case KEY_Q:
            if (pressed) app->togglePointerRelativeMode();
            break;

        case KEY_ESC:
        case KEY_CAPSLOCK:
            if (pressed)
            {
                app->_bRunning = false;
                LOG_OK("quit...\n");
            }
            break;

        case KEY_R:
            /*if (pressed) incCounter = 0;*/
            break;

        case KEY_F:
            if (pressed) app->toggleFullscreen();
            break;

        case KEY_V:
            if (pressed) app->toggleVSync();
            break;

        default:
            break;
    }
}

void
PlayerControls::procKeys(App* app)
{
    procMovements(app);

    if (pressedKeys[KEY_I])
    {
        frame::g_fov += 100.0f * f32(_deltaTime);
        LOG_OK("fov: %.3f}\n", frame::g_fov);
    }
    if (pressedKeys[KEY_O])
    {
        frame::g_fov -= 100.0f * f32(_deltaTime);
        LOG_OK("fov: %.3f\n", frame::g_fov);
    }
    if (pressedKeys[KEY_Z])
    {
        /*f64 inc = pressedKeys[KEY_LEFTSHIFT] ? (-4.0) : 4.0;*/
        /*x += inc * deltaTime;*/
        /*LOG_OK("x: %.3f\n", x);*/
    }
    if (pressedKeys[KEY_X])
    {
        /*f64 inc = pressedKeys[KEY_LEFTSHIFT] ? (-4.0) : 4.0;*/
        /*y += inc * deltaTime;*/
        /*LOG_OK("y: %.3f\n", y);*/
    }
    if (pressedKeys[KEY_C])
    {
        /*f64 inc = pressedKeys[KEY_LEFTSHIFT] ? (-4.0) : 4.0;*/
        /*z += inc * deltaTime;*/
        /*LOG_OK("z: %.3f\n", z);*/
    }
}

void
PlayerControls::procMovements([[maybe_unused]] App* c)
{
    f64 moveSpeed = _moveSpeed * _deltaTime;

    v3 combinedMove {};
    if (pressedKeys[KEY_W])
    {
        v3 forward {_front.x, 0.0f, _front.z};
        combinedMove += (v3Norm(forward));
    }
    if (pressedKeys[KEY_S])
    {
        v3 forward {_front.x, 0.0f, _front.z};
        combinedMove -= (v3Norm(forward));
    }
    if (pressedKeys[KEY_A])
    {
        v3 left = v3Norm(v3Cross(_front, _up));
        combinedMove -= (left);
    }
    if (pressedKeys[KEY_D])
    {
        v3 left = v3Norm(v3Cross(_front, _up));
        combinedMove += (left);
    }
    if (pressedKeys[KEY_SPACE])
    {
        combinedMove += _up;
    }
    if (pressedKeys[KEY_LEFTCTRL])
    {
        combinedMove -= _up;
    }
    f32 len = v3Length(combinedMove);
    if (len > 0) combinedMove = v3Norm(combinedMove, len);

    if (pressedKeys[KEY_LEFTSHIFT])
        moveSpeed *= 3;
    if (pressedKeys[KEY_LEFTALT])
        moveSpeed /= 3;

    _pos += combinedMove * f32(moveSpeed);
}

void
PlayerControls::updateDeltaTime()
{
    _currTime = adt::timeNowS();
    _deltaTime = _currTime - _lastFrameTime;
    _lastFrameTime = _currTime;
}

void 
PlayerControls::updateView()
{
    _view = m4LookAt(_pos, _pos + _front, _up);
}

void 
PlayerControls::updateProj(f32 fov, f32 aspect, f32 near, f32 far)
{
    _proj = m4Pers(fov, aspect, near, far);
}

} /* namespace controls */
