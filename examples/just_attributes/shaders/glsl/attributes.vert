#version 450


layout(location = 0) in vec4 pos;
layout(location = 1) in vec4 norm;
layout(location = 2) in vec3 col;

out vec3 evalCol;
out vec4 evalNorm;

void main() {
    evalCol = col;
    evalNorm = normalize(norm);
    gl_Position = pos;
}
