#include "Model.hh"
#include "Shader.hh"
#include "ThreadPool.hh"
#include "colors.hh"
#include "frame.hh"
#include "math.hh"
#include "Text.hh"
#include "AtomicArenaAllocator.hh"
#include "AllocatorPool.hh"

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

static adt::AllocatorPool<adt::AtomicArenaAllocator, ASSET_MAX_COUNT> apAssets;

static Shader shTex;
static Shader shBitMap;
static Shader shColor;
static Shader shCubeDepth;
static Shader shOmniDirShadow;
static Shader shSkyBox;
static Model mSphere(apAssets.get(adt::SIZE_8M));
static Model mSponza(apAssets.get(adt::SIZE_8M * 2));
static Model mBackpack(apAssets.get(adt::SIZE_8M));
static Model mCube(apAssets.get(adt::SIZE_1M));
static Texture tAsciiMap(apAssets.get(adt::SIZE_1M));
static Text textFPS;
static Ubo uboProjView;
static CubeMap cmCubeMap;
static CubeMap cmSkyBox;

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
    shCubeDepth.loadShaders("shaders/shadows/cubeMap/cubeMapDepth.vert", "shaders/shadows/cubeMap/cubeMapDepth.geom", "shaders/shadows/cubeMap/cubeMapDepth.frag");
    shOmniDirShadow.loadShaders("shaders/shadows/cubeMap/omniDirShadow.vert", "shaders/shadows/cubeMap/omniDirShadow.frag");
    shSkyBox.loadShaders("shaders/skybox.vert", "shaders/skybox.frag");

    shTex.use();
    shTex.setI("tex0", 0);

    shBitMap.use();
    shBitMap.setI("tex0", 0);

    shOmniDirShadow.use();
    shOmniDirShadow.setI("uDiffuseTexture", 0);
    shOmniDirShadow.setI("uDepthMap", 1);

    shSkyBox.use();
    shSkyBox.setI("uSkyBox", 0);

    uboProjView.createBuffer(sizeof(m4) * 2, GL_DYNAMIC_DRAW);
    uboProjView.bindBlock(&shTex, "ubProjView", 0);
    uboProjView.bindBlock(&shColor, "ubProjView", 0);
    uboProjView.bindBlock(&shOmniDirShadow, "ubProjView", 0);
    uboProjView.bindBlock(&shSkyBox, "ubProjView", 0);

    cmCubeMap = makeCubeShadowMap(1024, 1024);

    adt::String skyboxImgs[6] {
        "test-assets/skybox/right.bmp",
        "test-assets/skybox/left.bmp",
        "test-assets/skybox/top.bmp",
        "test-assets/skybox/bottom.bmp",
        "test-assets/skybox/front.bmp",
        "test-assets/skybox/back.bmp"
    };

    cmSkyBox = makeSkyBox(skyboxImgs);

    textFPS = Text("", adt::size(_fpsStrBuff), 0, 0, GL_DYNAMIC_DRAW);

    adt::ArenaAllocator allocScope(adt::SIZE_1K);
    adt::ThreadPool tp(&allocScope);
    tp.start();

    /* unbind before creating threads */
    self->unbindGlContext();

    TexLoadArg bitMap {&tAsciiMap, "test-assets/FONT.bmp", TEX_TYPE::DIFFUSE, false, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR_MIPMAP_NEAREST, self};

    ModelLoadArg sphere {&mSphere, "test-assets/models/icosphere/gltf/untitled.gltf", GL_STATIC_DRAW, GL_MIRRORED_REPEAT, self};
    ModelLoadArg sponza {&mSponza, "test-assets/models/Sponza/Sponza.gltf", GL_STATIC_DRAW, GL_MIRRORED_REPEAT, self};
    ModelLoadArg backpack {&mBackpack, "test-assets/models/backpack/scene.gltf", GL_STATIC_DRAW, GL_MIRRORED_REPEAT, self};
    ModelLoadArg cube {&mCube, "test-assets/models/cube/gltf/cube.gltf", GL_STATIC_DRAW, GL_MIRRORED_REPEAT, self};

    tp.submit(TextureSubmit, &bitMap);
    tp.submit(ModelSubmit, &sphere);
    tp.submit(ModelSubmit, &sponza);
    tp.submit(ModelSubmit, &backpack);
    tp.submit(ModelSubmit, &cube);

    tp.wait();
    /* restore context after assets are loaded */
    self->bindGlContext();

    tp.destroy();
    allocScope.freeAll();
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
renderFPSCounter(adt::Allocator* pAlloc)
{
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

        textFPS.update(pAlloc, _fpsStrBuff, 0, 0);
    }

    textFPS.draw();
}

void
renderSkyBox()
{
    m4 view = m3(player.view); /* remove translation */

    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);

    shSkyBox.use();
    shSkyBox.setM4("uViewNoTranslate", view);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cmSkyBox.tex);
    mCube.draw(DRAW::NONE);

    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
}

void
renderScene(adt::Allocator* pAlloc, Shader* sh)
{
    m4 m = m4Iden();
    mSponza.drawGraph(pAlloc, DRAW::ALL ^ DRAW::NORM, sh, "uModel", "uNormalMatrix", m);

    m = m4Iden();
    m *= m4Translate(m, {0, 0.5, 0});
    m *= m4Scale(m, 0.002);
    m = m4RotY(m, toRad(90));
    mBackpack.drawGraph(pAlloc, DRAW::ALL ^ DRAW::NORM, sh, "uModel", "uNormalMatrix", m);
}

void
mainLoop(App* self)
{
    adt::ArenaAllocator allocFrame(adt::SIZE_8M);

    while (self->bRunning)
    {
        {
            player.updateDeltaTime();
            player.procMouse();
            player.procKeys(self);

            self->procEvents();

            f32 aspect = f32(self->wWidth) / f32(self->wHeight);
            constexpr f32 shadowAspect = 1024.0f / 1024.0f;

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            player.updateProj(toRad(fov), aspect, 0.01f, 100.0f);
            player.updateView();
            /* copy both proj and view in one go */
            uboProjView.bufferData(&player, 0, sizeof(m4) * 2);

            v3 lightPos {cosf((f32)player.currTime) * 6.0f, 3.0f, sinf((f32)player.currTime) * 1.1f};
            constexpr v3 lightColor(colors::whiteSmoke);
            constexpr f32 nearPlane = 0.01f, farPlane = 25.0f;
            m4 shadowProj = m4Pers(toRad(90), shadowAspect, nearPlane, farPlane);
            CubeMapProjections tmShadows(shadowProj, lightPos);

            /* render scene to depth cubemap */
            glViewport(0, 0, cmCubeMap.width, cmCubeMap.height);
            glBindFramebuffer(GL_FRAMEBUFFER, cmCubeMap.fbo);
            glClear(GL_DEPTH_BUFFER_BIT);

            shCubeDepth.use();
            for (u32 i = 0; i < adt::size(tmShadows.tms); i++)
            {
                char buff[30] {};
                snprintf(buff, sizeof(buff), "uShadowMatrices[%d]", i);
                shCubeDepth.setM4(buff, tmShadows[i]);
            }
            shCubeDepth.setV3("uLightPos", lightPos);
            shCubeDepth.setF("uFarPlane", farPlane);
            glActiveTexture(GL_TEXTURE1);
            glCullFace(GL_FRONT);
            renderScene(&allocFrame, &shCubeDepth);
            glCullFace(GL_BACK);

            /* reset viewport */
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, self->wWidth, self->wHeight);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            /* draw skybox prior to everything else */
            renderSkyBox();

            /*render scene as normal using the generated depth map */
            shOmniDirShadow.use();
            shOmniDirShadow.setV3("uLightPos", lightPos);
            shOmniDirShadow.setV3("uLightColor", lightColor);
            shOmniDirShadow.setV3("uViewPos", player.pos);
            shOmniDirShadow.setF("uFarPlane", farPlane);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cmCubeMap.tex);
            renderScene(&allocFrame, &shOmniDirShadow);

            shColor.use();
            m4 m = m4Translate(m4Iden(), lightPos);
            m = m4Scale(m, 0.07f);
            shColor.setV3("uColor", lightColor);
            mSphere.drawGraph(&allocFrame, DRAW::APPLY_TM, &shColor, "uModel", "", m);

            renderFPSCounter(&allocFrame);
        }

        allocFrame.reset();
        self->swapBuffers();

        _fpsCount++;
    }

    allocFrame.freeAll();
    apAssets.freeAll();
}

} /* namespace frame */
