#version 450

layout (location = 0) out vec3 outUVW;

void main()
{
    const float initialRadius = 1.0;
    const float inclination = 0.6;

    int triIndex = gl_VertexIndex / 3;
    float radius = initialRadius / exp2(float(triIndex));
    float vertexInclination = inclination + float((gl_VertexIndex % 3) * 2 + triIndex % 2) * radians(60);

    outUVW = vec3(gl_VertexIndex % 3 == 1, gl_VertexIndex % 3 == 2, triIndex);

    gl_Position = vec4( vec2(sin(vertexInclination), cos(vertexInclination)) * radius , 1.0 , 1.0);
}
