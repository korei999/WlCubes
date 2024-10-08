#include <immintrin.h>

#include "App.hh"
#include "ArenaAllocator.hh"
#include "Texture.hh"
#include "frame.hh"
#include "logs.hh"
#include "parser/Binary.hh"

/* Bitmap file format
 *
 * SECTION
 * Address:Bytes	Name
 *
 * HEADER:
 *	  0:	2		"BM" magic number
 *	  2:	4		file size
 *	  6:	4		junk
 *	 10:	4		Starting address of image data
 * BITMAP HEADER:
 *	 14:	4		header size
 *	 18:	4		width  (signed)
 *	 22:	4		height (signed)
 *	 26:	2		Number of color planes
 *	 28:	2		Bits per pixel
 *	[...]
 * [OPTIONAL COLOR PALETTE, NOT PRESENT IN 32 BIT BITMAPS]
 * BITMAP DATA:
 *	DATA:	X	Pixels
 */

void
Texture::load(adt::String path, TEX_TYPE type, bool flip, GLint texMode, GLint magFilter, GLint minFilter)
{
#ifdef TEXTURE
    LOG_OK("loading '%.*s' texture...\n", (int)path.size, path.pData);
#endif

    if (_id != 0)
    {
        LOG_WARN("id != 0: '%d'\n", _id);
        return;
    }

    adt::ArenaAllocator aAlloc(adt::SIZE_1M * 5);
    TextureData img = loadBMP(&aAlloc, path, flip);

    _texPath = path;
    _type = type;

    adt::Array<u8> pixels = img.aData;

    setTexture(pixels.data(), texMode, img.format, img.width, img.height, magFilter, minFilter);
    _width = img.width;
    _height = img.height;

#ifdef TEXTURE
    LOG(OK, "%.*s: id: %d, texMode: %d\n", (int)path.size, path.pData, id, format);
#endif

    aAlloc.freeAll();
}

void
Texture::bind(GLint glTexture)
{
    glActiveTexture(glTexture);
    glBindTexture(GL_TEXTURE_2D, _id);
}

void
Texture::setTexture(u8* pData, GLint texMode, GLint format, GLsizei width, GLsizei height, GLint magFilter, GLint minFilter)
{
    mtx_lock(&gl::mtxGlContext);
    frame::g_app->bindGlContext();

    glGenTextures(1, &_id);
    glBindTexture(GL_TEXTURE_2D, _id);
    /* set the texture wrapping parameters */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texMode);
    /* set texture filtering parameters */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);

    /* NOTE: swapping bits on load is not nessesary?
     * glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
     * glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED); */

    /* load image, create texture and generate mipmaps */
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, pData);
    glGenerateMipmap(GL_TEXTURE_2D);

    frame::g_app->unbindGlContext();
    mtx_unlock(&gl::mtxGlContext);
}

CubeMapProjections::CubeMapProjections(const m4 proj, const v3 pos)
    : _tms{
        proj * m4LookAt(pos, pos + v3( 1, 0, 0), v3(0,-1, 0)),
        proj * m4LookAt(pos, pos + v3(-1, 0, 0), v3(0,-1, 0)),
        proj * m4LookAt(pos, pos + v3( 0, 1, 0), v3(0, 0, 1)),
        proj * m4LookAt(pos, pos + v3( 0,-1, 0), v3(0, 0,-1)),
        proj * m4LookAt(pos, pos + v3( 0, 0, 1), v3(0,-1, 0)),
        proj * m4LookAt(pos, pos + v3( 0, 0,-1), v3(0,-1, 0))
    } {}

ShadowMap
makeShadowMap(const int width, const int height)
{
    GLenum none = GL_NONE;
    ShadowMap res {};
    res.width = width;
    res.height = height;

    glGenTextures(1, &res.tex);
    glBindTexture(GL_TEXTURE_2D, res.tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	f32 borderColor[] {1.0, 1.0, 1.0, 1.0};
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLint defFramebuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &defFramebuffer);
    /* set up fbo */
    glGenFramebuffers(1, &res.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, res.fbo);

    glDrawBuffers(1, &none);
    glReadBuffer(GL_NONE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, res.tex, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, res.tex);

    if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER))
        LOG_FATAL("glCheckFramebufferStatus != GL_FRAMEBUFFER_COMPLETE\n"); 

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return res;
}

CubeMap
makeCubeShadowMap(const int width, const int height)
{
    GLuint depthCubeMap;
    glGenTextures(1, &depthCubeMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMap);

    for (GLuint i = 0; i < 6; i++)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                     0, GL_DEPTH_COMPONENT, width, height,
                     0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubeMap, 0);
    GLenum none = GL_NONE;
    glDrawBuffers(1, &none);
    glReadBuffer(GL_NONE);

    if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER))
        LOG_FATAL("glCheckFramebufferStatus != GL_FRAMEBUFFER_COMPLETE\n"); 

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return {fbo, depthCubeMap, width, height};
}

CubeMap
makeSkyBox(adt::String sFaces[6])
{
    adt::ArenaAllocator alloc(adt::SIZE_1M * 6);
    CubeMap cmNew {};

    glGenTextures(1, &cmNew.tex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cmNew.tex);

    for (u32 i = 0; i < 6; i++)
    {
        TextureData tex = loadBMP(&alloc, sFaces[i], true);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                     0, tex.format, tex.width, tex.height,
                     0, tex.format, GL_UNSIGNED_BYTE, tex.aData.data());
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    alloc.freeAll();

    return cmNew;
}

TextureData
loadBMP(adt::Allocator* pAlloc, adt::String path, bool flip)
{
    u32 imageDataAddress;
    u32 width;
    u32 height;
    u32 nPixels;
    u16 bitDepth;
    u8 byteDepth;

    parser::Binary p(pAlloc, path);
    auto BM = p.readString(2);

    if (BM != "BM")
        LOG_FATAL("BM: %.*s, bmp file should have 'BM' as first 2 bytes\n", (int)BM._size, BM._pData);

    p.skipBytes(8);
    imageDataAddress = p.read32();

#ifdef TEXTURE
    LOG_OK("imageDataAddress: %u\n", imageDataAddress);
#endif

    p.skipBytes(4);
    width = p.read32();
    height = p.read32();
#ifdef TEXTURE
    LOG_OK("width: %d, height: %d\n", width, height);
#endif

    [[maybe_unused]] auto colorPlane = p.read16();
#ifdef TEXTURE
    LOG_OK("colorPlane: %d\n", colorPlane);
#endif

    GLint format = GL_RGB;
    bitDepth = p.read16();
#ifdef TEXTURE
    LOG_OK("bitDepth: %u\n", bitDepth);
#endif

    switch (bitDepth)
    {
        case 24:
            format = GL_RGB;
            break;

        case 32:
            format = GL_RGBA;
            break;

        default:
            LOG_WARN("support only for 32 and 24 bit bmp's, read '%u', setting to GL_RGB\n", bitDepth);
            break;
    }

    bitDepth = 32; /* use RGBA anyway */
    nPixels = width * height;
    byteDepth = bitDepth / 8;
#ifdef TEXTURE
    LOG_OK("nPixels: %lu, byteDepth: %u\n", nPixels, byteDepth);
#endif
    adt::Array<u8> pixels(pAlloc, nPixels * byteDepth);

    p.setPos(imageDataAddress);

    switch (format)
    {
        default:
        case GL_RGB:
            flipCpyBGRtoRGBA(pixels.data(), (u8*)(&p[p.start]), width, height, flip);
            format = GL_RGBA;
            break;

        case GL_RGBA:
            flipCpyBGRAtoRGBA(pixels.data(), (u8*)(&p[p.start]), width, height, flip);
            break;
    }

    return {
        .aData = pixels,
        .width = width,
        .height = height,
        .bitDepth = bitDepth,
        .format = format
    };
}

#ifdef DEBUG
[[maybe_unused]] static void
printPack(adt::String s, __m128i m)
{
    u32 f[4];
    memcpy(f, &m, sizeof(f));
    COUT("'%.*s': %08x, %08x, %08x, %08x\n", s._size, s.data(), f[0], f[1], f[2], f[3]);
};
#endif

[[maybe_unused]] static u32
swapRedBlueBits(u32 col)
{
    u32 r = col & 0x00'ff'00'00;
    u32 b = col & 0x00'00'00'ff;
    return (col & 0xff'00'ff'00) | (r >> (4*4)) | (b << (4*4));
};

void
flipCpyBGRAtoRGBA(u8* dest, u8* src, int width, int height, bool vertFlip)
{
    int f = vertFlip ? -(height - 1) : 0;
    int inc = vertFlip ? 2 : 0;

    u32* d = (u32*)(dest);
    u32* s = (u32*)(src);

    for (int y = 0; y < height; y++, f += inc)
    {
        for (int x = 0; x < width; x += 4)
        {
            __m128i pack = _mm_loadu_si128((__m128i*)(&s[y*width + x]));
            __m128i redBits = _mm_and_si128(pack, _mm_set1_epi32(0x00'ff'00'00));
            __m128i blueBits = _mm_and_si128(pack, _mm_set1_epi32(0x00'00'00'ff));
            pack = _mm_and_si128(pack, _mm_set1_epi32(0xff'00'ff'00));

            /* https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html#techs=SSE_ALL&ig_expand=3975,627,305,2929,627&cats=Shift */
            redBits = _mm_bsrli_si128(redBits, 2); /* shift 2 because: 'dst[127:0] := a[127:0] << (tmp*8)' */
            blueBits = _mm_bslli_si128(blueBits, 2);

            pack = _mm_or_si128(_mm_or_si128(pack, redBits), blueBits);
            _mm_storeu_si128((__m128i*)(&d[(y-f)*width + x]), pack);
        }
    }
};

void
flipCpyBGRtoRGB(u8* dest, u8* src, int width, int height, bool vertFlip)
{
    int f = vertFlip ? -(height - 1) : 0;
    int inc = vertFlip ? 2 : 0;

    constexpr int nComponents = 3;
    width = width * nComponents;

    auto at = [=](int x, int y, int z) -> int {
        return y*width + x + z;
    };

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x += nComponents)
        {
            dest[at(x, y-f, 0)] = src[at(x, y, 2)];
            dest[at(x, y-f, 1)] = src[at(x, y, 1)];
            dest[at(x, y-f, 2)] = src[at(x, y, 0)];
        }
        f += inc;
    }
};

void
flipCpyBGRtoRGBA(u8* dest, u8* src, int width, int height, bool vertFlip)
{
    int f = vertFlip ? -(height - 1) : 0;
    int inc = vertFlip ? 2 : 0;

    constexpr int rgbComp = 3;
    constexpr int rgbaComp = 4;

    int rgbWidth = width * rgbComp;
    int rgbaWidth = width * rgbaComp;

    auto at = [](int width, int x, int y, int z) -> int {
        return y*width + x + z;
    };

    for (int y = 0; y < height; y++)
    {
        for (int xSrc = 0, xDest = 0; xSrc < rgbWidth; xSrc += rgbComp, xDest += rgbaComp)
        {
            dest[at(rgbaWidth, xDest, y-f, 0)] = src[at(rgbWidth, xSrc, y, 2)];
            dest[at(rgbaWidth, xDest, y-f, 1)] = src[at(rgbWidth, xSrc, y, 1)];
            dest[at(rgbaWidth, xDest, y-f, 2)] = src[at(rgbWidth, xSrc, y, 0)];
            dest[at(rgbaWidth, xDest, y-f, 3)] = 0xff;
        }
        f += inc;
    }
};
