#pragma once

#include "App.hh"
#include "controls.hh"

namespace frame
{

constexpr u32 ASSET_MAX_COUNT = 512;

extern App* g_app;
extern controls::PlayerControls g_player;
extern f32 g_fov;
extern f32 g_uiWidth;
extern f32 g_uiHeight;

void run(App* pApp);

} /* namespace frame */
