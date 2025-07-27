#version 450

// G-buffer inputs
layout(binding = 0) uniform sampler2D gPosition;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D gAlbedoSpec;

in vec2 TexCoord;
out vec4 FragColor;

void main()
{
    vec3 FragPos = texture(gPosition, TexCoord).rgb;
    vec3 Normal  = normalize(texture(gNormal, TexCoord).rgb);
    vec4 Albedo  = texture(gAlbedoSpec, TexCoord);

    FragColor = Albedo;
}
