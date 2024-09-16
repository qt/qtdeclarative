#version 440

layout(location = 0) in vec4 qt_VertexPosition;
layout(location = 1) in vec2 qt_VertexTexCoord;

layout(location = 0) out vec2 qt_TexCoord;

layout(std140, binding = 0) uniform buf {
#if QSHADER_VIEW_COUNT >= 2
    mat4 qt_Matrix[QSHADER_VIEW_COUNT];
#else
    mat4 qt_Matrix;
#endif
    float opacity;
};

void main()
{
    qt_TexCoord = qt_VertexTexCoord;
#if QSHADER_VIEW_COUNT >= 2
    gl_Position = qt_Matrix[gl_ViewIndex] * qt_VertexPosition;
#else
    gl_Position = qt_Matrix * qt_VertexPosition;
#endif
}
