#version 450

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec4 inColor;


layout (location = 0) out vec4 outFragColor;

void main() {
    vec4 outColor = inColor;
    vec3 lightDir = normalize(vec3(1.0, 0.0, 0.5));
    float diffuse = (dot(lightDir, inNormal) + 1.0f) / 2.0f;

    outFragColor = outColor * diffuse;
}
