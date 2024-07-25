#version 320 es
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex;

uniform mat4 uProj;
uniform mat4 uModel;

out vec2 vsTex;

void
main()
{
    gl_Position = uModel * vec4(aPos, 1.0);
    vsTex = aTex;
}
