#version 440

layout(location = 0) in vec4 qt_VertexPosition;
layout(location = 1) in vec2 qt_VertexTexCoord;

layout(location = 0) out vec2 qt_TexCoord;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float opacity;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    qt_TexCoord = qt_VertexTexCoord;
    gl_Position = ubuf.qt_Matrix * qt_VertexPosition;
}
