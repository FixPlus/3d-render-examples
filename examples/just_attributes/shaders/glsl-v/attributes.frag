#version 450

layout (location = 0) in vec4 inNormal;
layout (location = 1) in vec3 inColor;

layout (location = 0)out vec4 colorOut;
void main() {
    vec4 lightDir = normalize(vec4(1.0, 0.0, 0.5, 0.0));
    float diffuse = (dot(lightDir, inNormal) + 1.0f) * 0.5f;
    diffuse = diffuse * 0.6f + 0.4f;
    colorOut = vec4(inColor * diffuse, 1.0f);

}
