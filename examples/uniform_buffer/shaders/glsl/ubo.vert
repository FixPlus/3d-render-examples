#version 450


layout (location = 0) in vec4 inPos;
layout (location = 1) in vec4 inNormal;


layout (location = 0) out vec4 outNormal;
layout (location = 1) out vec3 outColor;

layout (binding = 0) uniform MeshTransform{
    mat4 matrix;
    vec3 color;
} transform;

layout (binding = 1) uniform Camera{
    mat4 cameraSpace;
    mat4 perspective;
} camera;

void main() {
    outNormal = transform.matrix * inNormal;

    outColor = transform.color;

    gl_Position = camera.perspective * camera.cameraSpace * transform.matrix * inPos;
}