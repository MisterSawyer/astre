#version 450 core

in vec3 WorldPos;

layout(location=0) out vec4 FragColor;

uniform vec4 uColor = vec4(1.0, 0.8, 0.2, 1.0);

void main()
{
    FragColor = uColor;
}