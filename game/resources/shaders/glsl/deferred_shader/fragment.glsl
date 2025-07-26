#version 450

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

layout(location = 0) out vec3 gPosition;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out vec4 gAlbedoSpec;

uniform vec4 uColor;
uniform sampler2D uTexture;
uniform bool useTexture;

void main()
{
    gPosition = FragPos;
    gNormal   = normalize(Normal);
    vec4 baseColor = useTexture ? texture(uTexture, TexCoord) : uColor;
    gAlbedoSpec = baseColor; // .rgb = color, .a = specular factor (optional)
}
