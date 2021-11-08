#version 450

layout (location = 1) in vec3 evalCol;
layout (location = 0) in vec4 evalNorm;

layout (location = 0) out vec4 outFragCol;

void main() {
    vec4 lightDir = normalize(vec4(1.0, 0.0, 0.5, 0.0));
    float diffuse = (dot(lightDir, evalNorm) + 1.0f) / 2.0f;
    diffuse = diffuse * 0.6f + 0.4f;
    outFragCol = vec4(evalCol * diffuse, 1.0f);
}

