#include "AtomicArena.hh"
#include "Model.hh"
#include "Shader.hh"
#include "ThreadPool.hh"
#include "colors.hh"
#include "frame.hh"
#include "gl/gl.hh"
#include "logs.hh"
#include "math.hh"

namespace frame
{

#ifdef FPS_COUNTER
static f64 _prevTime;
#endif

controls::PlayerControls player {
    .pos {0.0, 1.0, 1.0},
    .moveSpeed = 4.0,
    .mouse {.sens = 0.07}
};

f32 fov = 90.0f;

static adt::AtomicArena allocAssets(adt::SIZE_1M * 100);

Shader shTex;
Model mSponza(&allocAssets);
Model mBackpack(&allocAssets);
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

    adt::ThreadPool tp(&allocAssets);
    tp.start();

    /* unbind before creating threads */
    app->unbindGlContext();

    struct ModelLoadArg
    {
        Model* p;
        adt::String path;
        GLint drawMode;
        GLint texMode;
        App* c;
    };

    ModelLoadArg sponza {&mSponza, "test-assets/models/Sponza/Sponza.gltf", GL_STATIC_DRAW, GL_MIRRORED_REPEAT, app};
    ModelLoadArg backpack {&mBackpack, "test-assets/models/backpack/scene.gltf", GL_STATIC_DRAW, GL_MIRRORED_REPEAT, app};

    auto load = [](void* p) -> int {
        auto a = *(ModelLoadArg*)p;
        a.p->load(a.path, a.drawMode, a.texMode, a.c);
        return 0;
    };

    tp.submit(load, &sponza);
    tp.submit(load, &backpack);

    tp.wait();
    /* restore context after assets are loaded */
    app->bindGlContext();

    tp.stop();
    tp.destroy();
}

void
run(App* app)
{
    adt::Arena allocFrame(adt::SIZE_8M);

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

            v3 lightPos {cosf(player.currTime) * 6.0f, 3, sinf(player.currTime) * 1.1f};
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
            mSponza.drawGraph(&allocFrame, DRAW::DIFF | DRAW::APPLY_TM | DRAW::APPLY_NM, &shTex, "uModel", "uNormalMatrix", m);
            m = m4Iden();
            m *= m4Translate(m, {0, 0.5, 0});
            m *= m4Scale(m, 0.002);
            m = m4RotY(m, toRad(90));
            mBackpack.drawGraph(&allocFrame, DRAW::DIFF | DRAW::APPLY_TM | DRAW::APPLY_NM, &shTex, "uModel", "uNormalMatrix", m);
        }

        allocFrame.reset();
        app->swapBuffers();

#ifdef FPS_COUNTER
        _fpsCount++;
#endif
    }

    allocFrame.freeAll();
    allocAssets.freeAll();
    allocFrame.destroy();
    allocAssets.destroy();
}

} /* namespace frame */
