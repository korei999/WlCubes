#pragma once

#include "Array.hh"
#include "gl/gl.hh"
#include "String.hh"

struct TextCharQuad
{
    f32 vs[24];
};

struct Text
{
    adt::String str;
    u32 maxSize;
    GLuint vao;
    GLuint vbo;
    GLuint vboSize;

    Text() = default;
    Text(adt::String s, u32 size, int x, int y, GLint drawMode) : str(s), maxSize(size) {
        this->genMesh(size, x, y, drawMode);
    }

    void draw();
    void update(adt::VIAllocator* pAlloc, adt::String s, int x, int y);

private:

    void genMesh(u32 size, int x, int y, GLint drawMode);
    adt::Array<TextCharQuad> genBuffer(adt::VIAllocator* pAlloc, adt::String s, u32 size, int x, int y);
};
