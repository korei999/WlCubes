#pragma once

#include "math.hh"
#include "ultratypes.h"
#include "App.hh"
#include "gl/gl.hh"
#include "Allocator.hh"

enum TEX_TYPE : int
{
    DIFFUSE = 0,
    NORMAL
};

struct Texture
{
    adt::VIAllocator* pAlloc;
    adt::String texPath;
    u32 width;
    u32 height;
    GLuint id = 0;
    enum TEX_TYPE type;

    Texture() = default;
    Texture(adt::VIAllocator* p) : pAlloc(p) {}
    Texture(adt::VIAllocator* p, adt::String path, TEX_TYPE type, bool flip, GLint texMode, App* c) : pAlloc(p) {
        this->loadBMP(path, type, flip, texMode, GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST, c);
    }

    void loadBMP(adt::String path, TEX_TYPE type, bool flip, GLint texMode, GLint magFilter, GLint minFilter, App* c);
    void bind(GLint glTexture);

private:
    void setTexture(u8* data, GLint texMode, GLint format, GLsizei width, GLsizei height, GLint magFilter, GLint minFilter, App* c);
};

struct ShadowMap
{
    GLuint fbo;
    GLuint tex;
    int width;
    int height;
};

struct CubeMap : ShadowMap
{
};

struct CubeMapProjections
{
    m4 tms[6];

    CubeMapProjections(const m4 projection, const v3 position);

    m4& operator[](size_t i) { return tms[i]; }
};

ShadowMap makeShadowMap(const int width, const int height);
CubeMap makeCubeShadowMap(const int width, const int height);
void flipCpyBGRAtoRGBA(u8* dest, u8* src, int width, int height, bool vertFlip);
void flipCpyBGRtoRGB(u8* dest, u8* src, int width, int height, bool vertFlip);
void flipCpyBGRtoRGBA(u8* dest, u8* src, int width, int height, bool vertFlip);
