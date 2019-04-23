#version 440

layout(location = 0) in vec2 coords;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    float t;
} ubuf;

void main()
{
    float i = 1. - (pow(abs(coords.x), 4.) + pow(abs(coords.y), 4.));
    i = smoothstep(ubuf.t - 0.8, ubuf.t + 0.8, i);
    i = floor(i * 20.) / 20.;
    fragColor = vec4(coords * .5 + .5, i, i);
}
