#version 400

out vec4 frag_colour;

in vec3 uv;

void main(){

    int triIndex = int(uv.z);

    frag_colour = vec4( (vec2(1.0) + cos(uv.xy * 50.0) * 0.6 + sin(uv.yx * 100.0 + 30.0) * 0.4) * 0.5,
    0.5 , 1.0f);

    if(triIndex % 2 == 0)
        frag_colour = vec4(frag_colour.g, frag_colour.b, frag_colour.r, 1.0f);
}