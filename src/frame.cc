#include "AllocatorPool.hh"
#include "ArenaAllocator.hh"
#include "Model.hh"
#include "Shader.hh"
#include "Text.hh"
#include "ThreadPool.hh"
#include "colors.hh"
#include "frame.hh"
#include "math.hh"

namespace frame
{

static void mainLoop(App* pApp);

f32 g_fov = 90.0f;
f32 g_uiWidth = 192.0f;
f32 g_uiHeight = (g_uiWidth * 9.0f) / 16.0f;

static f64 s_prevTime;
static int s_fpsCount = 0;
static char s_fpsStrBuff[40] {};

controls::PlayerControls player({0.0f, 1.0f, 1.0f}, 4.0, 0.07);

static adt::AllocatorPool<adt::ArenaAllocator, ASSET_MAX_COUNT> s_apAssets;

static Shader s_shTex;
static Shader s_shBitMap;
static Shader s_shColor;
static Shader s_shCubeDepth;
static Shader s_shOmniDirShadow;
static Shader s_shSkyBox;

static Model s_mSphere(s_apAssets.get(adt::SIZE_1M));
static Model s_mSponza(s_apAssets.get(adt::SIZE_8M * 2));
static Model s_mBackpack(s_apAssets.get(adt::SIZE_8M));
static Model s_mCube(s_apAssets.get(adt::SIZE_1M));

static Texture s_tAsciiMap(s_apAssets.get(adt::SIZE_1M));

static Text s_textFPS;
static Text s_textTest;

static CubeMap s_cmCubeMap;
static CubeMap s_cmSkyBox;

static Ubo s_uboProjView;

void
prepareDraw(App* pApp)
{
    pApp->bindGlContext();
    pApp->showWindow();

#ifdef DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(gl::debugCallback, pApp);
#endif

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    v4 gray = v4Color(0x444444FF);
    glClearColor(gray.r, gray.g, gray.b, gray.a);

    s_shTex.loadShaders("shaders/simpleTex.vert", "shaders/simpleTex.frag");
    s_shColor.loadShaders("shaders/simpleUB.vert", "shaders/simple.frag");
    s_shBitMap.loadShaders("shaders/font/font.vert", "shaders/font/font.frag");
    s_shCubeDepth.loadShaders("shaders/shadows/cubeMap/cubeMapDepth.vert", "shaders/shadows/cubeMap/cubeMapDepth.geom", "shaders/shadows/cubeMap/cubeMapDepth.frag");
    s_shOmniDirShadow.loadShaders("shaders/shadows/cubeMap/omniDirShadow.vert", "shaders/shadows/cubeMap/omniDirShadow.frag");
    s_shSkyBox.loadShaders("shaders/skybox.vert", "shaders/skybox.frag");

    s_shTex.use();
    s_shTex.setI("tex0", 0);

    s_shBitMap.use();
    s_shBitMap.setI("tex0", 0);

    s_shOmniDirShadow.use();
    s_shOmniDirShadow.setI("uDiffuseTexture", 0);
    s_shOmniDirShadow.setI("uDepthMap", 1);

    s_shSkyBox.use();
    s_shSkyBox.setI("uSkyBox", 0);

    s_uboProjView.createBuffer(sizeof(m4) * 2, GL_DYNAMIC_DRAW);
    s_uboProjView.bindShader(&s_shTex, "ubProjView", 0);
    s_uboProjView.bindShader(&s_shColor, "ubProjView", 0);
    s_uboProjView.bindShader(&s_shOmniDirShadow, "ubProjView", 0);
    s_uboProjView.bindShader(&s_shSkyBox, "ubProjView", 0);

    s_cmCubeMap = makeCubeShadowMap(1024, 1024);

    adt::String skyboxImgs[6] {
        "test-assets/skybox/right.bmp",
        "test-assets/skybox/left.bmp",
        "test-assets/skybox/top.bmp",
        "test-assets/skybox/bottom.bmp",
        "test-assets/skybox/front.bmp",
        "test-assets/skybox/back.bmp"
    };

    s_cmSkyBox = makeSkyBox(skyboxImgs);

    s_textFPS = Text("", adt::size(s_fpsStrBuff), 0, 0, GL_DYNAMIC_DRAW);
    s_textTest = Text("", 256, 0, 0, GL_DYNAMIC_DRAW);

    adt::ArenaAllocator allocScope(adt::SIZE_1K);
    adt::ThreadPool tp(&allocScope);
    tp.start();

    /* unbind before creating threads */
    pApp->unbindGlContext();

    TexLoadArg bitMap {&s_tAsciiMap, "test-assets/bitmapFont2.bmp", TEX_TYPE::DIFFUSE, false, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST, pApp};

    ModelLoadArg sphere {&s_mSphere, "test-assets/models/icosphere/gltf/untitled.gltf", GL_STATIC_DRAW, GL_MIRRORED_REPEAT, pApp};
    ModelLoadArg sponza {&s_mSponza, "test-assets/models/Sponza/Sponza.gltf", GL_STATIC_DRAW, GL_MIRRORED_REPEAT, pApp};
    ModelLoadArg backpack {&s_mBackpack, "test-assets/models/backpack/scene.gltf", GL_STATIC_DRAW, GL_MIRRORED_REPEAT, pApp};
    ModelLoadArg cube {&s_mCube, "test-assets/models/cube/gltf/cube.gltf", GL_STATIC_DRAW, GL_MIRRORED_REPEAT, pApp};

    tp.submit(TextureSubmit, &bitMap);
    tp.submit(ModelSubmit, &sphere);
    tp.submit(ModelSubmit, &sponza);
    tp.submit(ModelSubmit, &backpack);
    tp.submit(ModelSubmit, &cube);

    tp.wait();
    /* restore context after assets are loaded */
    pApp->bindGlContext();

    tp.destroy();
    allocScope.freeAll();

    pApp->setSwapInterval(1);
    pApp->toggleFullscreen();
}

void
run(App* pApp)
{
    pApp->_bRunning = true;
    /* FIXME: find better way to toggle this on startup */
    pApp->_bRelativeMode = false;
    pApp->togglePointerRelativeMode();
    pApp->bPaused = false;
    pApp->setCursorImage("default");

    s_prevTime = adt::timeNowS();

    prepareDraw(pApp);
    player.updateDeltaTime(); /* reset delta time before drawing */
    player.updateDeltaTime();

    mainLoop(pApp);
}

void
renderFPSCounter(adt::Allocator* pAlloc)
{
    m4 proj = m4Ortho(0.0f, g_uiWidth, 0.0f, g_uiHeight, -1.0f, 1.0f);
    s_shBitMap.use();
    s_tAsciiMap.bind(GL_TEXTURE0);
    s_shBitMap.setM4("uProj", proj);

    f64 _currTime = adt::timeNowS();
    if (_currTime >= s_prevTime + 1.0)
    {
        memset(s_fpsStrBuff, 0, adt::size(s_fpsStrBuff));
        snprintf(s_fpsStrBuff, adt::size(s_fpsStrBuff),
                "FPS: %u\nFrame time: %.3f ms", s_fpsCount, player._deltaTime);

        s_fpsCount = 0;
        s_prevTime = _currTime;

        s_textFPS.update(pAlloc, s_fpsStrBuff, 0, 0);
    }

    static bool bUpdateTextTest = true;
    if (bUpdateTextTest)
    {
        bUpdateTextTest = false;

        char buf[256] {};
        snprintf(buf, adt::size(buf), "Ascii test:\n!\"#$%%&'()*+,-./"
                                      "0123456789:;<=>?@ABCDEFGHIJKLMN"
                                      "OPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~");

        s_textTest.update(pAlloc, buf, 0, int(g_uiHeight - 2.0f*2.0f));
    }

    s_textFPS.draw();
    s_textTest.draw();
}

void
renderSkyBox()
{
    m4 view = m3(player._view); /* remove translation */

    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);

    s_shSkyBox.use();
    s_shSkyBox.setM4("uViewNoTranslate", view);
    glBindTexture(GL_TEXTURE_CUBE_MAP, s_cmSkyBox.tex);
    s_mCube.draw(DRAW::NONE);

    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
}

void
renderScene(adt::Allocator* pAlloc, Shader* sh)
{
    m4 m = m4Iden();
    s_mSponza.drawGraph(pAlloc, DRAW::ALL ^ DRAW::NORM, sh, "uModel", "uNormalMatrix", m);

    m = m4Iden();
    m *= m4Translate(m, {0, 0.5, 0});
    m *= m4Scale(m, 0.002f);
    m = m4RotY(m, toRad(90));
    s_mBackpack.drawGraph(pAlloc, DRAW::ALL ^ DRAW::NORM, sh, "uModel", "uNormalMatrix", m);
}

static void
mainLoop(App* pApp)
{
    adt::ArenaAllocator allocFrame(adt::SIZE_8M);

    while (pApp->_bRunning)
    {
        {
            player.updateDeltaTime();
            player.procMouse();
            player.procKeys(pApp);

            pApp->procEvents();

            f32 aspect = f32(pApp->_wWidth) / f32(pApp->_wHeight);
            constexpr f32 shadowAspect = 1024.0f / 1024.0f;

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            player.updateProj(toRad(g_fov), aspect, 0.01f, 100.0f);
            player.updateView();
            /* copy both proj and view in one go */
            s_uboProjView.bufferData(&player, 0, sizeof(m4) * 2);

            v3 lightPos {cosf((f32)player._currTime) * 6.0f, 3.0f, sinf((f32)player._currTime) * 1.1f};
            constexpr v3 lightColor(colors::whiteSmoke);
            constexpr f32 nearPlane = 0.01f, farPlane = 25.0f;
            m4 shadowProj = m4Pers(toRad(90), shadowAspect, nearPlane, farPlane);
            CubeMapProjections tmShadows(shadowProj, lightPos);

            /* render scene to depth cubemap */
            glViewport(0, 0, s_cmCubeMap.width, s_cmCubeMap.height);
            glBindFramebuffer(GL_FRAMEBUFFER, s_cmCubeMap.fbo);
            glClear(GL_DEPTH_BUFFER_BIT);

            s_shCubeDepth.use();
            for (u32 i = 0; i < adt::size(tmShadows._tms); i++)
            {
                char buff[30] {};
                snprintf(buff, sizeof(buff), "uShadowMatrices[%d]", i);
                s_shCubeDepth.setM4(buff, tmShadows[i]);
            }
            s_shCubeDepth.setV3("uLightPos", lightPos);
            s_shCubeDepth.setF("uFarPlane", farPlane);
            glActiveTexture(GL_TEXTURE1);
            glCullFace(GL_FRONT);
            renderScene(&allocFrame, &s_shCubeDepth);
            glCullFace(GL_BACK);

            /* reset viewport */
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, pApp->_wWidth, pApp->_wHeight);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            /* draw skybox prior to everything else */
            renderSkyBox();

            /*render scene as normal using the generated depth map */
            s_shOmniDirShadow.use();
            s_shOmniDirShadow.setV3("uLightPos", lightPos);
            s_shOmniDirShadow.setV3("uLightColor", lightColor);
            s_shOmniDirShadow.setV3("uViewPos", player._pos);
            s_shOmniDirShadow.setF("uFarPlane", farPlane);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, s_cmCubeMap.tex);
            renderScene(&allocFrame, &s_shOmniDirShadow);

            s_shColor.use();
            m4 m = m4Translate(m4Iden(), lightPos);
            m = m4Scale(m, 0.07f);
            s_shColor.setV3("uColor", lightColor);
            s_mSphere.drawGraph(&allocFrame, DRAW::APPLY_TM, &s_shColor, "uModel", "", m);

            renderFPSCounter(&allocFrame);
        }

        allocFrame.reset();
        pApp->swapBuffers();

        s_fpsCount++;
    }

    allocFrame.freeAll();
    s_apAssets.freeAll();
}

} /* namespace frame */
