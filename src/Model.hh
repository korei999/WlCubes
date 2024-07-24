#pragma once

#include <limits.h>

#include "gltf/gltf.hh"
#include "math.hh"
#include "Shader.hh"
#include "Texture.hh"
#include "App.hh"

enum class DRAW : int
{
    NONE     = 0,
    DIFF     = 1,      /* bind diffuse textures */
    NORM     = 1 << 1, /* bind normal textures */
    APPLY_TM = 1 << 2, /* apply transformation matrix */
    APPLY_NM = 1 << 3, /* generate and apply normal matrix */
    ALL      = INT_MAX
};

static inline bool
operator&(enum DRAW l, enum DRAW r)
{
    return static_cast<int>(l) & static_cast<int>(r);
}

static inline enum DRAW
operator|(enum DRAW l, enum DRAW r)
{
    return static_cast<enum DRAW>(static_cast<int>(l) | static_cast<int>(r));
}

static inline enum DRAW
operator^(enum DRAW l, enum DRAW r)
{
    return static_cast<enum DRAW>(static_cast<int>(l) ^ static_cast<int>(r));
}

struct Ubo
{
    GLuint id;
    u32 size;
    GLuint point;

    Ubo() = default;
    Ubo(u32 _size, GLint drawMode);

    void createBuffer(u32 _size, GLint drawMode);
    void bindBlock(Shader* sh, adt::String block, GLuint _point);
    void bufferData(void* data, u32 offset, u32 _size);
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
    adt::Allocator* pAlloc;
    adt::String savedPath;
    adt::Array<adt::Array<Mesh>> aaMeshes;
    gltf::Asset asset;

    Model(adt::Allocator* p) : pAlloc(p), aaMeshes(p), asset(p), aTmIdxs(p), aTmCounters(p) {}
    Model(adt::Allocator* p, adt::String path, GLint drawMode, GLint texMode, App* c) : Model(p) { this->load(path, drawMode, texMode, c); }

    void load(adt::String path, GLint drawMode, GLint texMode, App* c);
    void loadOBJ(adt::String path, GLint drawMode, GLint texMode, App* c);
    void loadGLTF(adt::String path, GLint drawMode, GLint texMode, App* c);
    void draw(enum DRAW flags, Shader* sh = nullptr, adt::String svUniform = "", adt::String svUniformM3Norm = "", const m4& tmGlobal = m4Iden());
    void drawGraph(adt::Allocator* pFrameAlloc, enum DRAW flags, Shader* sh, adt::String svUniform, adt::String svUniformM3Norm, const m4& tmGlobal);

private:
    void parseOBJ(adt::String path, GLint drawMode, GLint texMode, App* c);

    adt::Array<int> aTmIdxs; /* parents map */
    adt::Array<int> aTmCounters; /* map's sizes */
};

struct Quad
{
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLuint eboSize;

    void
    draw()
    {
        glBindVertexArray(this->vao);
        glDrawElements(GL_TRIANGLES, this->eboSize, GL_UNSIGNED_INT, nullptr);
    }
};

Quad makeQuad(GLint drawMode);
// Model getPlane(GLint drawMode = GL_STATIC_DRAW);
// Model getCube(GLint drawMode = GL_STATIC_DRAW);
// void drawQuad(const Model& q);
// void drawPlane(const Model& q);
// void drawCube(const Model& q);
