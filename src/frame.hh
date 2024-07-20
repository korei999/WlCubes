#pragma once

#include "App.hh"
#include "controls.hh"

namespace frame
{

extern controls::PlayerControls player;
extern f32 fov;

void run(App* pApp);

} /* namespace frame */
