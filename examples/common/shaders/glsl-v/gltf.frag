#version 450

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec4 inColor;


layout (binding = 2) uniform sampler2D colorMap;
layout (binding = 3) uniform sampler2D map3;
layout (binding = 4) uniform sampler2D normalMap;
layout (binding = 5) uniform sampler2D map5;
layout (binding = 6) uniform sampler2D map6;
layout (binding = 7) uniform sampler2D map7;
layout (binding = 8) uniform sampler2D map8;

layout (binding = 9) uniform MaterialInfo{
    float alphaCutoff;
    float metallicFactor;
    float roughnessFactor;
    float pad;
    vec4 baseColorFactor;
} material;


layout (location = 0) out vec4 outFragColor;

void main() {
    vec4 outColor = texture(colorMap, inUV) * material.baseColorFactor;
    vec3 lightDir = normalize(vec3(0.0, 0.5, -1.0));
    float diffuse = (dot(lightDir, inNormal) + 1.0f) / 2.0f;

    outFragColor = outColor * diffuse;
}
