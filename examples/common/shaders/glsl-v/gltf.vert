#version 450


layout (location = 0) in vec4 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec4 inColor;
layout (location = 4) in vec4 inJoint;
layout (location = 5) in vec4 inWeight;



layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec4 outColor;


layout (binding = 0) uniform Camera{
    mat4 cameraSpace;
    mat4 perspective;
} camera;

layout (binding = 1) uniform PrimitiveTransform{
    mat4 local;
} transform;

void main() {
    mat4 localCameraSpace = camera.cameraSpace * transform.local;
    outNormal = vec3(transform.local * vec4(inNormal, 0.0));

    outColor = inColor;
    outUV = inUV;

    gl_Position = camera.perspective * localCameraSpace * inPos;
}
