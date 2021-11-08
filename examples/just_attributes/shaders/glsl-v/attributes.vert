#version 450


layout (location = 0) in vec4 pos;
layout (location = 1) in vec4 normal;
layout (location = 2) in vec3 color;

layout (location = 0) out vec4 outNormal;
layout (location = 1) out vec3 outColor;

void main() {
    outNormal = normalize(normal);
    outColor = color;
    gl_Position = pos;
}
