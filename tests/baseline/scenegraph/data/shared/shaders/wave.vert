#version 440

layout(location = 0) in vec4 qt_Vertex;
layout(location = 1) in vec2 qt_MultiTexCoord0;

layout(location = 0) out vec2 qt_TexCoord0;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
} ubuf;

void main() {
    vec4 pos = qt_Vertex;
    pos.x += sin(qt_Vertex.y * 0.02) * 20.;
    pos.y += sin(qt_Vertex.x * 0.02) * 20.;
    gl_Position = ubuf.qt_Matrix * pos;
    qt_TexCoord0 = qt_MultiTexCoord0;
}
