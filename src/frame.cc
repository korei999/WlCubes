#include "AtomicArena.hh"
#include "Model.hh"
#include "Shader.hh"
#include "colors.hh"
#include "frame.hh"
#include "file.hh"
#include "logs.hh"
#include "math.hh"

#include "gl/gl.hh"

namespace frame
{

#ifdef FPS_COUNTER
static f64 _prevTime;
#endif

static adt::AtomicArena aarAssets(adt::SIZE_1M * 300);

controls::PlayerControls player {
    .pos {0.0, 1.0, 1.0},
    .moveSpeed = 4.0,
    .mouse {.sens = 0.07}
};

f32 fov = 90.0f;

Shader shTex;
Model mSponza(&aarAssets);
Ubo uboProjView;

void
prepareDraw(App* app)
{
    app->bindGlContext();
    app->showWindow();
    app->setSwapInterval(1);
    app->toggleFullscreen();

#ifdef DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(gl::debugCallback, app);
#endif

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    v4 gray = v4Color(0x444444FF);
    glClearColor(gray.r, gray.g, gray.b, gray.a);

    shTex.loadShaders("shaders/simpleTex.vert", "shaders/simpleTex.frag");

    shTex.use();
    shTex.setI("tex0", 0);

    uboProjView.createBuffer(sizeof(m4) * 2, GL_DYNAMIC_DRAW);
    uboProjView.bindBlock(&shTex, "ubProjView", 0);

    mSponza.load("test-assets/models/Sponza/Sponza.gltf", GL_STATIC_DRAW, GL_MIRRORED_REPEAT, app);

    /* unbind before creating threads */
    /*app->unbindGlContext();*/

    /* restore context after assets are loaded */
    /*app->bindGlContext();*/
}

void
run(App* app)
{
    adt::Arena arMainLoop(adt::SIZE_8M);

    app->bRunning = true;
    app->bRelativeMode = true;
    app->bPaused = false;
    app->setCursorImage("default");

#ifdef FPS_COUNTER
    _prevTime = adt::timeNowS();
#endif

    prepareDraw(app);
    player.updateDeltaTime(); /* reset delta time before drawing */
    player.updateDeltaTime();

    while (app->bRunning)
    {
#ifdef FPS_COUNTER
    static int _fpsCount = 0;
    f64 _currTime = adt::timeNowS();
    if (_currTime >= _prevTime + 1.0)
    {
        CERR("fps: %d, ms: %.3f\n", _fpsCount, player.deltaTime);
        _fpsCount = 0;
        _prevTime = _currTime;
    }
#endif
        app->procEvents();

        {
            player.updateDeltaTime();
            player.procMouse();
            player.procKeys(app);

            f32 aspect = static_cast<f32>(app->wWidth) / static_cast<f32>(app->wHeight);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            player.updateProj(toRad(fov), aspect, 0.01f, 100.0f);
            player.updateView();
            /* copy both proj and view in one go */
            uboProjView.bufferData(&player, 0, sizeof(m4) * 2);

            v3 lightPos {std::cosf(player.currTime) * 6.0f, 3, std::sinf(player.currTime) * 1.1f};
            constexpr v3 lightColor(colors::snow);
            f32 nearPlane = 0.01f, farPlane = 25.0f;

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            /* reset viewport */
            glViewport(0, 0, app->wWidth, app->wHeight);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            shTex.use();
            shTex.setV3("uLightPos", lightPos);
            shTex.setV3("uLightColor", lightColor);
            shTex.setV3("uViewPos", player.pos);
            shTex.setF("uFarPlane", farPlane);

            m4 m = m4Iden();
            mSponza.drawGraph(DRAW::DIFF | DRAW::APPLY_TM | DRAW::APPLY_NM, &shTex, "uModel", "uNormalMatrix", m);
        }

        app->swapBuffers();
#ifdef FPS_COUNTER
        _fpsCount++;
#endif
    }

    arMainLoop.freeAll();
    aarAssets.freeAll();
    aarAssets.destroy();
}

} /* namespace frame */
