#version 450 core

in vec3 WorldPos;

layout(location=0) out vec4 FragColor;

uniform vec4 uColor;

void main()
{
    FragColor = uColor;
}