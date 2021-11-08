#version 400


in vec3 evalCol;
in vec4 evalNorm;

out vec4 outFragCol;

void main() {
    vec4 lightDir = normalize(vec4(1.0, 0.0, 0.5, 0.0));
    float diffuse = (dot(lightDir, evalNorm) + 1.0f) / 2.0f;
    diffuse = diffuse * 0.6f + 0.4f;
    outFragCol = vec4(evalCol * diffuse, 1.0f);
}
