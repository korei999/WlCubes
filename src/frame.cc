#include "AtomicArena.hh"
#include "Model.hh"
#include "Shader.hh"
#include "ThreadPool.hh"
#include "colors.hh"
#include "frame.hh"
#include "gl/gl.hh"
#include "math.hh"
#include "Text.hh"

namespace frame
{

void mainLoop(App* self);

static f64 _prevTime;
static int _fpsCount = 0;
static char _fpsStrBuff[40] {};

controls::PlayerControls player {
    .pos {0.0, 1.0, 1.0},
    .moveSpeed = 4.0,
    .mouse {.sens = 0.07}
};

f32 fov = 90.0f;
f32 uiWidth = 150.0f;
f32 uiHeight = (uiWidth * 9.0f) / 16.0f;

static adt::AtomicArena allocAssets(adt::SIZE_1M * 200);

static Shader shTex;
static Shader shBitMap;
static Shader shColor;
static Model mSponza(&allocAssets);
static Model mBackpack(&allocAssets);
static Texture tAsciiMap(&allocAssets);
static Text textFPS;
static Quad mQuad;
static Ubo uboProjView;

void
prepareDraw(App* self)
{
    self->bindGlContext();
    self->showWindow();
    self->setSwapInterval(1);
    self->toggleFullscreen();

#ifdef DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(gl::debugCallback, self);
#endif

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    v4 gray = v4Color(0x444444FF);
    glClearColor(gray.r, gray.g, gray.b, gray.a);

    shTex.loadShaders("shaders/simpleTex.vert", "shaders/simpleTex.frag");
    shColor.loadShaders("shaders/simpleUB.vert", "shaders/simple.frag");
    shBitMap.loadShaders("shaders/font/font.vert", "shaders/font/font.frag");

    shTex.use();
    shTex.setI("tex0", 0);

    shBitMap.use();
    shBitMap.setI("tex0", 0);

    uboProjView.createBuffer(sizeof(m4) * 2, GL_DYNAMIC_DRAW);
    uboProjView.bindBlock(&shTex, "ubProjView", 0);
    uboProjView.bindBlock(&shColor, "ubProjView", 0);

    mQuad = makeQuad(GL_STATIC_DRAW);

    textFPS = Text("", adt::size(_fpsStrBuff), 0, 0, GL_DYNAMIC_DRAW);

    adt::Arena allocScope(adt::SIZE_1K);
    adt::ThreadPool tp(&allocScope);
    tp.start();

    /* unbind before creating threads */
    self->unbindGlContext();

    TexLoadArg bitMap {&tAsciiMap, "test-assets/FONT.bmp", TEX_TYPE::DIFFUSE, false, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR_MIPMAP_NEAREST, self};

    ModelLoadArg sponza {&mSponza, "test-assets/models/Sponza/Sponza.gltf", GL_STATIC_DRAW, GL_MIRRORED_REPEAT, self};
    ModelLoadArg backpack {&mBackpack, "test-assets/models/backpack/scene.gltf", GL_STATIC_DRAW, GL_MIRRORED_REPEAT, self};

    tp.submit(TextureSubmit, &bitMap);
    tp.submit(ModelSubmit, &sponza);
    tp.submit(ModelSubmit, &backpack);

    tp.wait();
    /* restore context after assets are loaded */
    self->bindGlContext();

    tp.stop();
    tp.destroy();

    allocScope.freeAll();
    allocScope.destroy();
}

void
run(App* self)
{
    self->bRunning = true;
    /* FIXME: find better way to toggle this on startup */
    self->bRelativeMode = true;
    self->bPaused = false;
    self->setCursorImage("default");

    _prevTime = adt::timeNowS();

    prepareDraw(self);
    player.updateDeltaTime(); /* reset delta time before drawing */
    player.updateDeltaTime();

    mainLoop(self);
}

void
mainLoop(App* self)
{
    adt::Arena allocFrame(adt::SIZE_8M);

    while (self->bRunning)
    {
        {
            player.updateDeltaTime();
            player.procMouse();
            player.procKeys(self);

            self->procEvents();

            f32 aspect = f32(self->wWidth) / f32(self->wHeight);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            [[maybe_unused]] constexpr f32 nearPlane = 0.01f, farPlane = 25.0f;

            player.updateProj(toRad(fov), aspect, 0.01f, 100.0f);
            player.updateView();
            /* copy both proj and view in one go */
            uboProjView.bufferData(&player, 0, sizeof(m4) * 2);

            v3 lightPos {cosf((f32)player.currTime) * 6.0f, 3.0f, sinf((f32)player.currTime) * 1.1f};
            constexpr v3 lightColor(colors::aqua);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            /* reset viewport */
            glViewport(0, 0, self->wWidth, self->wHeight);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            shTex.use();
            shTex.setV3("uLightPos", lightPos);
            shTex.setV3("uLightColor", lightColor);
            shTex.setV3("uViewPos", player.pos);
            shTex.setF("uFarPlane", farPlane);

            m4 m = m4Iden();
            mSponza.drawGraph(&allocFrame, DRAW::DIFF | DRAW::APPLY_TM | DRAW::APPLY_NM, &shTex, "uModel", "uNormalMatrix", m);

            m = m4Iden();
            m *= m4Translate(m, {0.0f, 0.5f, 0.0f});
            m *= m4Scale(m, 0.002f);
            m = m4RotY(m, toRad(90.0f));
            mBackpack.drawGraph(&allocFrame, DRAW::DIFF | DRAW::APPLY_TM | DRAW::APPLY_NM, &shTex, "uModel", "uNormalMatrix", m);


            /* fps counter */
            m4 proj = m4Ortho(0.0f, uiWidth, 0.0f, uiHeight, -1.0f, 1.0f);
            shBitMap.use();
            tAsciiMap.bind(GL_TEXTURE0);
            shBitMap.setM4("uProj", proj);

            f64 _currTime = adt::timeNowS();
            if (_currTime >= _prevTime + 1.0)
            {
                memset(_fpsStrBuff, 0, adt::size(_fpsStrBuff));
                snprintf(_fpsStrBuff, adt::size(_fpsStrBuff),
                         "FPS: %u\nFrame time: %.3f ms", _fpsCount, player.deltaTime);

                _fpsCount = 0;
                _prevTime = _currTime;

                textFPS.update(&allocFrame, _fpsStrBuff, 0, 0);
            }

            textFPS.draw();
        }

        allocFrame.reset();
        self->swapBuffers();

        _fpsCount++;
    }

    allocFrame.freeAll();
    allocAssets.freeAll();
    allocAssets.destroy();
}

} /* namespace frame */
