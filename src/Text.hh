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
    adt::String _str;
    u32 _maxSize;
    GLuint _vao;
    GLuint _vbo;
    GLuint _vboSize;

    Text() = default;
    Text(adt::String s, u32 size, int x, int y, GLint drawMode) : _str(s), _maxSize(size) {
        genMesh(x, y, drawMode);
    }

    void draw();
    void update(adt::Allocator* pAlloc, adt::String s, int x, int y);

private:

    void genMesh(int x, int y, GLint drawMode);
    adt::Array<TextCharQuad> genBuffer(adt::Allocator* pAlloc, adt::String s, u32 size, int x, int y);
};
