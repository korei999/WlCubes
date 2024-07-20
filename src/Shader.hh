#pragma once
#include "math.hh"
#include "gl/gl.hh"

#include "String.hh"

struct Shader
{
    GLuint id = 0;

    Shader() = default;
    Shader(adt::String vertShaderPath, adt::String fragShaderPath);
    Shader(adt::String vertShaderPath, adt::String geomShaderPath, adt::String fragShaderPath);
    Shader(Shader&& other);
    Shader(const Shader& other) = delete;
    ~Shader();

    void operator=(Shader&& other);

    void loadShaders(adt::String vertShaderPath, adt::String fragShaderPath);
    void loadShaders(adt::String vertexPath, adt::String geometryPath, adt::String fragmentPath);
    void use() const;
    void setM3(adt::String name, const m3& m);
    void setM4(adt::String name, const m4& m);
    void setV3(adt::String name, const v3& v);
    void setI(adt::String name, const GLint i);
    void setF(adt::String name, const f32 f);
    void queryActiveUniforms();

private:
    GLuint loadShader(GLenum type, adt::String path);
};
