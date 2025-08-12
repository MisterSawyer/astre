#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTex;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 WorldPos;

void main()
{
    vec4 wp = uModel * vec4(aPos, 1.0);
    WorldPos = wp.xyz;
    gl_Position = uProjection * uView * wp;
}