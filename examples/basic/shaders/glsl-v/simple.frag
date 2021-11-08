#version 450

layout (location = 0) in vec3 inUVW;

layout (location = 0) out vec4 outFragColor;


void main(){

    int triIndex = int(inUVW.z);

    outFragColor = vec4( (vec2(1.0) + cos(inUVW.xy * 50.0) * 0.6 + sin(inUVW.yx * 100.0 + 30.0) * 0.4) * 0.5,
                                0.5 , 1.0f);

    if(triIndex % 2 == 0)
        outFragColor = vec4(outFragColor.g, outFragColor.b, outFragColor.r, 1.0f);
}