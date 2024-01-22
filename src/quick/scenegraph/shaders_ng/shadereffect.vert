#version 440

layout(location = 0) in vec4 qt_Vertex;
layout(location = 1) in vec2 qt_MultiTexCoord0;

layout(location = 0) out vec2 qt_TexCoord0;

layout(std140, binding = 0) uniform qt_buf {
#if QSHADER_VIEW_COUNT >= 2
    mat4 qt_Matrix[QSHADER_VIEW_COUNT];
#else
    mat4 qt_Matrix;
#endif
    float qt_Opacity;
} qt_ubuf; // must use a name that does not clash with custom code when no uniform blocks

void main()
{
    qt_TexCoord0 = qt_MultiTexCoord0;
#if QSHADER_VIEW_COUNT >= 2
    gl_Position = qt_ubuf.qt_Matrix[gl_ViewIndex] * qt_Vertex;
#else
    gl_Position = qt_ubuf.qt_Matrix * qt_Vertex;
#endif
}
