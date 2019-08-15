#version 440

layout(location = 0) in vec2 pos;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    mat4 rotation;
    vec4 color;
    float pattern;
    int projection;
} ubuf;

void main(void)
{
    vec4 c = ubuf.color;
    c.xyz += pow(max(sin(pos.x + pos.y), 0.0), 2.0) * ubuf.pattern * 0.25;
    fragColor = c;
}
