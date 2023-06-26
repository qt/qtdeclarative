#version 440

layout(location = 0) in vec4 vertexCoord;
layout(location = 1) in vec4 vertexColor;

layout(location = 0) out vec2 coord;

layout(std140, binding = 0) uniform buf {
#if QSHADER_VIEW_COUNT >= 2
    mat4 matrix[QSHADER_VIEW_COUNT];
#else
    mat4 matrix;
#endif
    vec2 translationPoint;
    float angle;
    float opacity;
} ubuf;

void main()
{
    coord = vertexCoord.xy - ubuf.translationPoint;
#if QSHADER_VIEW_COUNT >= 2
    gl_Position = ubuf.matrix[gl_ViewIndex] * vertexCoord;
#else
    gl_Position = ubuf.matrix * vertexCoord;
#endif
}
