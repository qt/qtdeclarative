#version 440

layout(location = 0) in vec4 vertexCoord;
layout(location = 1) in vec4 vertexColor;

layout(location = 0) out float gradTabIndex;

layout(std140, binding = 0) uniform buf {
#if QSHADER_VIEW_COUNT >= 2
    mat4 matrix[QSHADER_VIEW_COUNT];
#else
    mat4 matrix;
#endif
    mat4 gradientMatrix;
    vec2 gradStart;
    vec2 gradEnd;
    float opacity;
} ubuf;

void main()
{
    vec2 gradVertexCoord = (ubuf.gradientMatrix * vertexCoord).xy;
    vec2 gradVec = ubuf.gradEnd - ubuf.gradStart;
    gradTabIndex = dot(gradVec, gradVertexCoord - ubuf.gradStart) / (gradVec.x * gradVec.x + gradVec.y * gradVec.y);
#if QSHADER_VIEW_COUNT >= 2
    gl_Position = ubuf.matrix[gl_ViewIndex] * vertexCoord;
#else
    gl_Position = ubuf.matrix * vertexCoord;
#endif
}
