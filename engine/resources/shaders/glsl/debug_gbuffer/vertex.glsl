#version 450

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aNormals;
layout(location = 2) in vec2 aUV;

out vec2 TexCoord;

void main()
{
    TexCoord = aUV;
    gl_Position = vec4(aPos, 0.0, 1.0); // convert to clip space
}
