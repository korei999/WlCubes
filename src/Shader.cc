#include "Shader.hh"
#include "logs.hh"
#include "Arena.hh"
#include "file.hh"

Shader::Shader(adt::String vertexPath, adt::String fragmentPath)
{
    loadShaders(vertexPath, fragmentPath);
}

Shader::Shader(adt::String vertexPath, adt::String geometryPath, adt::String fragmentPath)
{
    loadShaders(vertexPath, geometryPath, fragmentPath);
}

void
Shader::loadShaders(adt::String vertexPath, adt::String fragmentPath)
{
    GLint linked;
    GLuint vertex = this->loadShader(GL_VERTEX_SHADER, vertexPath);
    GLuint fragment = this->loadShader(GL_FRAGMENT_SHADER, fragmentPath);

    this->id = glCreateProgram();
    if (this->id == 0)
        LOG_FATAL("glCreateProgram failed: %d\n", this->id);

    glAttachShader(this->id, vertex);
    glAttachShader(this->id, fragment);

    glLinkProgram(this->id);
    glGetProgramiv(this->id, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLint infoLen = 0;
        char infoLog[255] {};
        glGetProgramiv(this->id, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1)
        {
            glGetProgramInfoLog(id, infoLen, nullptr, infoLog);
            LOG_FATAL("error linking program: %s\n", infoLog);
        }
        glDeleteProgram(id);
        LOG_FATAL("error linking program.\n");
    }

#ifdef DEBUG
    glValidateProgram(this->id);
#endif

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void
Shader::loadShaders(adt::String vertexPath, adt::String geometryPath, adt::String fragmentPath)
{
    GLint linked;
    GLuint vertex = this->loadShader(GL_VERTEX_SHADER, vertexPath);
    GLuint fragment = this->loadShader(GL_FRAGMENT_SHADER, fragmentPath);
    GLuint geometry = this->loadShader(GL_GEOMETRY_SHADER, geometryPath);

    this->id = glCreateProgram();
    if (this->id == 0)
        LOG_FATAL("glCreateProgram failed: %d\n", this->id);

    glAttachShader(this->id, vertex);
    glAttachShader(this->id, fragment);
    glAttachShader(this->id, geometry);

    glLinkProgram(this->id);
    glGetProgramiv(this->id, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLint infoLen = 0;
        glGetProgramiv(this->id, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1)
        {
            char infoLog[255] {};
            glGetProgramInfoLog(this->id, infoLen, nullptr, infoLog);
            LOG_FATAL("error linking program: %s\n", infoLog);
        }
        glDeleteProgram(this->id);
        LOG_FATAL("error linking program.\n");
    }

#ifdef DEBUG
    glValidateProgram(this->id);
#endif

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    glDeleteShader(geometry);
}

void
Shader::operator=(Shader&& other)
{
    this->id = other.id;
    other.id = 0;
}

Shader::~Shader()
{
    if (this->id)
    {
        glDeleteProgram(this->id);
        /*LOG(OK, "Shader '{}' deleted\n", this->id);*/
    }
}

GLuint
Shader::loadShader(GLenum type, adt::String path)
{
    GLuint shader;
    shader = glCreateShader(type);
    if (!shader)
        return 0;

    adt::Arena arena(adt::SIZE_8K);

    adt::String src = loadFile(&arena, path);
    const char* srcData = src.pData;

    glShaderSource(shader, 1, &srcData, nullptr);
    glCompileShader(shader);

    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1)
        {
            char infoLog[255] {};
            glGetShaderInfoLog(shader, infoLen, nullptr, infoLog);
            LOG_FATAL("error compiling shader '%.*s'\n%s\n", (int)path.size, path.pData, infoLog);
        }
        glDeleteShader(shader);
        return 0;
    }

    arena.freeAll();
    return shader;
}

void
Shader::use() const
{
    glUseProgram(this->id);
}

void
Shader::queryActiveUniforms()
{
    GLint maxUniformLen;
    GLint nUniforms;

    glGetProgramiv(this->id, GL_ACTIVE_UNIFORMS, &nUniforms);
    glGetProgramiv(this->id, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxUniformLen);

    char uniformName[255] {};
    LOG_OK("queryActiveUniforms for '%d':\n", this->id);

    for (int i = 0; i < nUniforms; i++)
    {
        GLint size;
        GLenum type;
        adt::String typeName;

        glGetActiveUniform(this->id, i, maxUniformLen, nullptr, &size, &type, uniformName);
        switch (type)
        {
            case GL_FLOAT:
                typeName = "GL_FLOAT";
                break;

            case GL_FLOAT_VEC2:
                typeName = "GL_FLOAT_VEC2";
                break;

            case GL_FLOAT_VEC3:
                typeName = "GL_FLOAT_VEC3";
                break;

            case GL_FLOAT_VEC4:
                typeName = "GL_FLOAT_VEC4";
                break;

            case GL_FLOAT_MAT4:
                typeName = "GL_FLOAT_MAT4";
                break;

            case GL_FLOAT_MAT3:
                typeName = "GL_FLOAT_MAT3";
                break;

            case GL_SAMPLER_2D:
                typeName = "GL_SAMPLER_2D";
                break;

            default:
                typeName = "unknown";
                break;
        }
        LOG_OK("\tuniformName: '%s', type: '%.*s'\n", uniformName, (int)typeName.size, typeName.pData);
    }
}

void 
Shader::setM4(adt::String name, const m4& m)
{
    GLint ul;
    ul = glGetUniformLocation(this->id, name.data());
    glUniformMatrix4fv(ul, 1, GL_FALSE, (GLfloat*)m.e);
}

void 
Shader::setM3(adt::String name, const m3& m)
{
    GLint ul;
    ul = glGetUniformLocation(this->id, name.data());
    glUniformMatrix3fv(ul, 1, GL_FALSE, (GLfloat*)m.e);
}


void
Shader::setV3(adt::String name, const v3& v)
{
    GLint ul;
    ul = glGetUniformLocation(this->id, name.data());
    glUniform3fv(ul, 1, (GLfloat*)v.e);
}

void
Shader::setI(adt::String name, const GLint i)
{
    GLint ul;
    ul = glGetUniformLocation(this->id, name.data());
    glUniform1i(ul, i);
}

void
Shader::setF(adt::String name, const f32 f)
{
    GLint ul;
    ul = glGetUniformLocation(this->id, name.data());
    glUniform1f(ul, f);
}
