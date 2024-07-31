#pragma once

#include <limits.h>

#include "gltf/gltf.hh"
#include "math.hh"
#include "Shader.hh"
#include "Texture.hh"
#include "App.hh"

enum DRAW : int
{
    NONE     = 0,
    DIFF     = 1,      /* bind diffuse textures */
    NORM     = 1 << 1, /* bind normal textures */
    APPLY_TM = 1 << 2, /* apply transformation matrix */
    APPLY_NM = 1 << 3, /* generate and apply normal matrix */
    ALL      = INT_MAX
};

inline bool
operator&(enum DRAW l, enum DRAW r)
{
    return int(l) & int(r);
}

inline enum DRAW
operator|(enum DRAW l, enum DRAW r)
{
    return (enum DRAW)(int(l) | int(r));
}

inline enum DRAW
operator^(enum DRAW l, enum DRAW r)
{
    return (enum DRAW)((int)(l) ^ (int)(r));
}

struct Ubo
{
    GLuint _id;
    u32 _size;
    GLuint _point;

    Ubo() = default;
    Ubo(u32 size, GLint drawMode);

    void createBuffer(u32 size, GLint drawMode);
    void bindBlock(Shader* sh, adt::String block, GLuint point);
    void bufferData(void* data, u32 offset, u32 size);
};

struct Materials
{
    Texture diffuse;
    Texture normal;
};

struct MeshData
{
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLuint eboSize;

    Materials materials;

    adt::String name;
};

struct Mesh
{
    MeshData meshData;

    enum gltf::COMPONENT_TYPE indType;
    enum gltf::PRIMITIVES mode;
    u32 triangleCount;
};

struct Model
{
    adt::Allocator* _pAlloc;
    adt::String _sSavedPath;
    adt::Array<adt::Array<Mesh>> _aaMeshes;
    gltf::Asset _asset;

    Model(adt::Allocator* p) : _pAlloc(p), _aaMeshes(p), _asset(p), _aTmIdxs(p), _aTmCounters(p) {}
    Model(adt::Allocator* p, adt::String path, GLint drawMode, GLint texMode, App* c) : Model(p) {}

    void load(adt::String path, GLint drawMode, GLint texMode, App* c);
    void loadOBJ(adt::String path, GLint drawMode, GLint texMode, App* c);
    void loadGLTF(adt::String path, GLint drawMode, GLint texMode, App* c);
    void draw(enum DRAW flags, Shader* sh = nullptr, adt::String svUniform = "", adt::String svUniformM3Norm = "", const m4& tmGlobal = m4Iden());
    void drawGraph(adt::Allocator* pFrameAlloc, enum DRAW flags, Shader* sh, adt::String svUniform, adt::String svUniformM3Norm, const m4& tmGlobal);

private:
    void parseOBJ(adt::String path, GLint drawMode, GLint texMode, App* c);

    adt::Array<int> _aTmIdxs; /* parents map */
    adt::Array<int> _aTmCounters; /* map's sizes */
};

struct Quad
{
    GLuint _vao;
    GLuint _vbo;
    GLuint _ebo;
    GLuint _eboSize;

    void
    draw()
    {
        glBindVertexArray(_vao);
        glDrawElements(GL_TRIANGLES, _eboSize, GL_UNSIGNED_INT, nullptr);
    }
};

struct ModelLoadArg
{
    Model* p;
    adt::String path;
    GLint drawMode;
    GLint texMode;
    App* c;
};

inline int
ModelSubmit(void* p)
{
    auto a = *(ModelLoadArg*)p;
    a.p->load(a.path, a.drawMode, a.texMode, a.c);
    return 0;
};

Quad makeQuad(GLint drawMode);
// Model getPlane(GLint drawMode = GL_STATIC_DRAW);
// Model getCube(GLint drawMode = GL_STATIC_DRAW);
// void drawQuad(const Model& q);
// void drawPlane(const Model& q);
// void drawCube(const Model& q);
