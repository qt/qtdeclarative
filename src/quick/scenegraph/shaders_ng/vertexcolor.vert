#version 440

layout(location = 0) in vec4 vertexCoord;
layout(location = 1) in vec4 vertexColor;

layout(location = 0) out vec4 color;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    float opacity;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    gl_Position = ubuf.matrix * vertexCoord;
    color = vertexColor * ubuf.opacity;
}
