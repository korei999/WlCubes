#pragma once

#include "App.hh"
#include "controls.hh"

namespace frame
{

constexpr u32 ASSET_MAX_COUNT = 512;

extern controls::PlayerControls player;
extern f32 fov;
extern f32 uiWidth;
extern f32 uiHeight;

void run(App* pApp);

} /* namespace frame */
