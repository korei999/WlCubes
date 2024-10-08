#pragma once

#include "Allocator.hh"
#include "Array.hh"
#include "String.hh"
#include "gl/gl.hh"
#include "math.hh"
#include "ultratypes.h"

enum TEX_TYPE : int
{
    DIFFUSE = 0,
    NORMAL
};

struct TextureData
{
    adt::Array<u8> aData;
    u32 width;
    u32 height;
    u16 bitDepth;
    GLint format;
};

struct Texture
{
    adt::Allocator* _pAlloc;
    adt::String _texPath;
    u32 _width;
    u32 _height;
    GLuint _id = 0;
    enum TEX_TYPE _type;

    Texture() = default;
    Texture(adt::Allocator* p) : _pAlloc(p) {}
    Texture(adt::Allocator* p, adt::String path, TEX_TYPE type, bool flip, GLint texMode) : _pAlloc(p) {
        load(path, type, flip, texMode, GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST);
    }

    void load(adt::String path, TEX_TYPE type, bool flip, GLint texMode, GLint magFilter, GLint minFilter);
    void bind(GLint glTexture);

private:
    void setTexture(u8* data, GLint texMode, GLint format, GLsizei width, GLsizei height, GLint magFilter, GLint minFilter);
};

struct TexLoadArg
{
    Texture* self;
    adt::String path;
    TEX_TYPE type;
    bool flip;
    GLint texMode;
    GLint magFilter;
    GLint minFilter;
};

inline int
TextureSubmit(void* p)
{
    auto a = *(TexLoadArg*)p;
    a.self->load(a.path, a.type, a.flip, a.texMode, a.magFilter, a.minFilter);
    return 0;
}

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
    m4 _tms[6];

    CubeMapProjections(const m4 projection, const v3 position);

    m4& operator[](size_t i) { return _tms[i]; }
};

ShadowMap makeShadowMap(const int width, const int height);
CubeMap makeCubeShadowMap(const int width, const int height);
CubeMap makeSkyBox(adt::String sFaces[6]);
TextureData loadBMP(adt::Allocator* pAlloc, adt::String path, bool flip);
void flipCpyBGRAtoRGBA(u8* dest, u8* src, int width, int height, bool vertFlip);
void flipCpyBGRtoRGB(u8* dest, u8* src, int width, int height, bool vertFlip);
void flipCpyBGRtoRGBA(u8* dest, u8* src, int width, int height, bool vertFlip);
