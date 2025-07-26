#version 450

#define UNKNOWN_LIGHT_TYPE 0
#define LIGHT_TYPE_DIRECTIONAL 1
#define LIGHT_TYPE_POINT 2
#define LIGHT_TYPE_SPOT 3

struct GPULight {
    vec4 position;      // xyz: position, w: unused
    vec4 direction;     // xyz: direction, w: type
    vec4 color;         // rgb: color, w: intensity
    vec4 attenuation;   // x: constant, y: linear, z: quadratic, w: unused
    vec2 cutoff;        // x: inner cos, y: outer cos
    vec2 castShadows;   // vec2 to have natural padding
};

layout(std430, binding = 2) buffer LightBuffer {
    GPULight lights[];
};
uniform int lightCount;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform vec4 uColor;
uniform sampler2D uTexture;
uniform bool useTexture;

vec3 calculateLight(GPULight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 lightDir;
    float attenuation = 1.0;
    float diff = 0.0;

    if (int(light.direction.w) == LIGHT_TYPE_DIRECTIONAL) {
        lightDir = normalize(-light.direction.xyz);
    } else {
        lightDir = normalize(light.position.xyz - fragPos);

        float dist = length(light.position.xyz - fragPos);
        attenuation = 1.0 / (light.attenuation.x + light.attenuation.y * dist + light.attenuation.z * dist * dist);

        if (int(light.direction.w) == LIGHT_TYPE_SPOT) {
            float theta = dot(lightDir, normalize(-light.direction.xyz));
            float epsilon = light.cutoff.x - light.cutoff.y;
            float intensity = clamp((theta - light.cutoff.y) / epsilon, 0.0, 1.0);
            attenuation *= intensity;
        }
    }

    diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.color.rgb * light.color.w * diff * attenuation;
    return diffuse;
}

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(-FragPos); // Assuming camera at origin

    vec4 baseColor = useTexture ? texture(uTexture, TexCoord) : uColor;
    vec3 lighting = vec3(0.0);

    for (int i = 0; i < lightCount; ++i) {
        lighting += calculateLight(lights[i], norm, FragPos, viewDir);
    }

    vec3 resultColor = baseColor.rgb * lighting;
    FragColor = vec4(resultColor, baseColor.a);
}
