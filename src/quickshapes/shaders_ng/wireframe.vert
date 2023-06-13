#version 440

layout(location = 0) in vec4 vertexCoord;
layout(location = 1) in vec3 vertexBarycentric;
layout(location = 0) out vec3 barycentric;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    barycentric = vertexBarycentric;
    gl_Position = ubuf.qt_Matrix * vertexCoord;
}
