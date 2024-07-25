#pragma once

#include "gl/gl.hh"
#include "String.hh"

struct Text
{
    adt::String str;
    GLuint vao;
    GLuint vbo;
    GLuint vboSize;

    Text() = default;
    Text(adt::String s, int x, int y) : str(s) { this->genMesh(x, y); }

    void draw();

private:
    void genMesh(int x, int y);
};
