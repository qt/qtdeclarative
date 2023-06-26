#version 440

layout(location = 0) in vec4 vertexCoord;
layout(location = 1) in vec3 vertexBarycentric;
layout(location = 0) out vec3 barycentric;

layout(std140, binding = 0) uniform buf {
#if QSHADER_VIEW_COUNT >= 2
    mat4 qt_Matrix[QSHADER_VIEW_COUNT];
#else
    mat4 qt_Matrix;
#endif
} ubuf;

void main()
{
    barycentric = vertexBarycentric;
#if QSHADER_VIEW_COUNT >= 2
    gl_Position = ubuf.qt_Matrix[gl_ViewIndex] * vertexCoord;
#else
    gl_Position = ubuf.qt_Matrix * vertexCoord;
#endif
}
