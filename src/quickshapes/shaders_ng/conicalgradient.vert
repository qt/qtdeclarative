#version 440

layout(location = 0) in vec4 vertexCoord;
layout(location = 1) in vec4 vertexColor;

layout(location = 0) out vec2 coord;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    vec2 translationPoint;
    float angle;
    float opacity;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    coord = vertexCoord.xy - ubuf.translationPoint;
    gl_Position = ubuf.matrix * vertexCoord;
}
