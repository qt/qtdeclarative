#version 440

layout(location = 0) in vec4 qt_Vertex;
layout(location = 1) in vec2 qt_MultiTexCoord0;

layout(location = 0) out vec2 qt_TexCoord0;

layout(std140, binding = 0) uniform qt_buf {
    mat4 qt_Matrix;
    float qt_Opacity;
} qt_ubuf; // must use a name that does not clash with custom code when no uniform blocks

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    qt_TexCoord0 = qt_MultiTexCoord0;
    gl_Position = qt_ubuf.qt_Matrix * qt_Vertex;
}
