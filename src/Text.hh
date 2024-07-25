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
    GLuint vao;
    GLuint vbo;
    GLuint vboSize;

    Text() = default;
    Text(adt::String s, int x, int y, GLint drawMode) : str(s) { this->genMesh(x, y, drawMode, s.size); }

    void draw();
    void update(adt::VIAllocator* pAlloc, adt::String s, int x, int y);

private:

    void genMesh(int x, int y, GLint drawMode, u32 size);
    adt::Array<TextCharQuad> genBuffer(adt::VIAllocator* pAlloc, adt::String s, int x, int y, u32 size);
};
