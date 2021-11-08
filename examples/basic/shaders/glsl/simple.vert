#version 400

in vec3 vp;

out vec3 uv;


void main()
{
    const float initialRadius = 1.0;
    const float inclination = 1.5;

    int triIndex = gl_VertexID / 3;
    float radius = initialRadius / exp2(float(triIndex));
    float vertexInclination = inclination + float((gl_VertexID % 3) * 2 + triIndex % 2) * radians(-60);

    uv = vec3(gl_VertexID % 3 == 1, gl_VertexID % 3 == 2, triIndex);
    gl_Position = vec4( vec2(sin(vertexInclination), cos(vertexInclination)) * radius , 1.0 , 1.0);
}
