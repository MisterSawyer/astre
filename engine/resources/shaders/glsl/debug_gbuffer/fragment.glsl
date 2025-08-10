#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform int debugMode; // 0=position, 1=normal, 2=albedo

void main()
{
    if (debugMode == 0)
    {
        vec3 pos = texture(gPosition, TexCoord).rgb;
        FragColor = vec4(pos * 0.1, 1.0); // scale for visibility
    }
    else if (debugMode == 1)
    {
        vec3 normal = normalize(texture(gNormal, TexCoord).rgb);
        FragColor = vec4(normal * 0.5 + 0.5, 1.0); // map -1..1 to 0..1
    }
    else if (debugMode == 2)
    {
        vec4 albedo = texture(gAlbedoSpec, TexCoord);
        FragColor = albedo;
    }
    else
    {
        FragColor = vec4(1, 0, 1, 1); // magenta for invalid mode
    }
}
