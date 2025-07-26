#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform vec4 uColor;
uniform sampler2D uTexture;
uniform bool useTexture;

void main()
{
    vec4 baseColor = useTexture ? texture(uTexture, TexCoord) : uColor;
    FragColor = baseColor;
}