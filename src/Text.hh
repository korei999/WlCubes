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
    Text(adt::String s) : str(s) { this->genMesh(); }

    void draw();

private:
    void genMesh();
};
