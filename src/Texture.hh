#pragma once

#include "math.hh"
#include "ultratypes.h"
#include "App.hh"
#include "gl/gl.hh"
#include "allocator.hh"

enum TEX_TYPE : int
{
    DIFFUSE = 0,
    NORMAL
};

struct Texture
{
    adt::BaseAllocator* pAlloc;
    GLuint id = 0;
    enum TEX_TYPE type;

    adt::String texPath;

    Texture() = default;
    Texture(adt::BaseAllocator* p) : pAlloc(p) {}
    Texture(adt::BaseAllocator* p, adt::String path, TEX_TYPE type, bool flip, GLint texMode, App* c) : pAlloc(p) { this->loadBMP(path, type, flip, texMode, c); }

    void loadBMP(adt::String path, TEX_TYPE type, bool flip, GLint texMode, App* c);
    void bind(GLint glTexture);

private:
    void setTexture(u8* data, GLint texMode, GLint format, GLsizei width, GLsizei height, App* c);
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

ShadowMap createShadowMap(const int width, const int height);
CubeMap createCubeShadowMap(const int width, const int height);
void flipCpyBGRAtoRGBA(u8* dest, u8* src, int width, int height, bool vertFlip);
void flipCpyBGRtoRGB(u8* dest, u8* src, int width, int height, bool vertFlip);
void flipCpyBGRtoRGBA(u8* dest, u8* src, int width, int height, bool vertFlip);
