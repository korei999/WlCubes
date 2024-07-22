#include <math.h>

#include "controls.hh"
#include "frame.hh"
#include "logs.hh"

#ifdef __linux__
#include <linux/input-event-codes.h>
#elif _WIN32

#endif

#undef near//;
#undef far//;

namespace controls
{

bool pressedKeys[300] {};

void
PlayerControls::procMouse()
{
    f64 offsetX = (this->mouse.relX - this->mouse.prevRelX) * this->mouse.sens;
    f64 offsetY = (this->mouse.prevRelY - this->mouse.relY) * this->mouse.sens;

    this->mouse.prevRelX = this->mouse.relX;
    this->mouse.prevRelY = this->mouse.relY;

    this->mouse.yaw += offsetX;
    this->mouse.pitch += offsetY;

    if (this->mouse.pitch > 89.9)
        this->mouse.pitch = 89.9;
    if (this->mouse.pitch < -89.9)
        this->mouse.pitch = -89.9;

    front = v3Norm({
        static_cast<f32>(cos(toRad(this->mouse.yaw)) * cos(toRad(this->mouse.pitch))),
        static_cast<f32>(sin(toRad(this->mouse.pitch))),
        static_cast<f32>(sin(toRad(this->mouse.yaw)) * cos(toRad(this->mouse.pitch)))
    });

    this->right = v3Norm(v3Cross(this->front, this->up));
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
                app->bRunning = false;
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
    this->procMovements(app);

    if (pressedKeys[KEY_I])
    {
        frame::fov += 100.0f * static_cast<f32>(this->deltaTime);
        LOG_OK("fov: %.3f}\n", frame::fov);
    }
    if (pressedKeys[KEY_O])
    {
        frame::fov -= 100.0f * static_cast<f32>(this->deltaTime);
        LOG_OK("fov: %.3f\n", frame::fov);
    }
    if (pressedKeys[KEY_Z])
    {
        /*f64 inc = pressedKeys[KEY_LEFTSHIFT] ? (-4.0) : 4.0;*/
        /*x += inc * this->deltaTime;*/
        /*LOG_OK("x: %.3f\n", x);*/
    }
    if (pressedKeys[KEY_X])
    {
        /*f64 inc = pressedKeys[KEY_LEFTSHIFT] ? (-4.0) : 4.0;*/
        /*y += inc * this->deltaTime;*/
        /*LOG_OK("y: %.3f\n", y);*/
    }
    if (pressedKeys[KEY_C])
    {
        /*f64 inc = pressedKeys[KEY_LEFTSHIFT] ? (-4.0) : 4.0;*/
        /*z += inc * this->deltaTime;*/
        /*LOG_OK("z: %.3f\n", z);*/
    }
}

void
PlayerControls::procMovements([[maybe_unused]] App* c)
{
    f64 moveSpeed = this->moveSpeed * this->deltaTime;

    v3 combinedMove {};
    if (pressedKeys[KEY_W])
    {
        v3 forward {this->front.x, 0.0f, this->front.z};
        combinedMove += (v3Norm(forward));
    }
    if (pressedKeys[KEY_S])
    {
        v3 forward {this->front.x, 0.0f, this->front.z};
        combinedMove -= (v3Norm(forward));
    }
    if (pressedKeys[KEY_A])
    {
        v3 left = v3Norm(v3Cross(this->front, this->up));
        combinedMove -= (left);
    }
    if (pressedKeys[KEY_D])
    {
        v3 left = v3Norm(v3Cross(this->front, this->up));
        combinedMove += (left);
    }
    if (pressedKeys[KEY_SPACE])
    {
        combinedMove += this->up;
    }
    if (pressedKeys[KEY_LEFTCTRL])
    {
        combinedMove -= this->up;
    }

    f32 len = v3Length(combinedMove);
    if (len > 0) combinedMove = v3Norm(combinedMove, len);

    this->pos += combinedMove * static_cast<f32>(moveSpeed);
}

void
PlayerControls::updateDeltaTime()
{
    this->currTime = adt::timeNowS();
    this->deltaTime = this->currTime - this->lastFrameTime;
    this->lastFrameTime = this->currTime;
}

void 
PlayerControls::updateView()
{
    this->view = m4LookAt(pos, pos + front, up);
}

void 
PlayerControls::updateProj(f32 fov, f32 aspect, f32 near, f32 far)
{
    this->proj = m4Pers(fov, aspect, near, far);
}

} /* namespace controls */
