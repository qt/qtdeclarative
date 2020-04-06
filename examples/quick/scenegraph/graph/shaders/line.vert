#version 440

layout(location = 0) in vec4 pos;
layout(location = 1) in float t;

layout(location = 0) out float vT;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    vec4 color;
    float qt_Opacity;
    float size;
    float spread;
};

out gl_PerVertex { vec4 gl_Position; };

void main(void)
{
    vec4 adjustedPos = pos;
    adjustedPos.y += (t * size);
    gl_Position = qt_Matrix * adjustedPos;

    vT = t;
}
