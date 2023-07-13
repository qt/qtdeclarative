#version 440

layout(location = 0) in vec4 vertexCoord;
layout(location = 1) in vec2 inA;
layout(location = 2) in vec2 inB;
layout(location = 3) in vec2 inC;
layout(location = 4) in vec2 inHG;
layout(location = 5) in float inoffset;

layout(location = 0) out vec4 P;
layout(location = 1) out vec2 A;
layout(location = 2) out vec2 B;
layout(location = 3) out vec2 C;
layout(location = 4) out vec2 HG;
layout(location = 5) out float offset;


layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;

    float matrixScale;
    float opacity;
    float reserved2;
    float reserved3;

    vec4 strokeColor;

    float strokeWidth;
    float reserved4;
    float reserved5;
    float reserved6;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    P = vertexCoord;
    A = inA;
    B = inB;
    C = inC;
    HG = inHG;
    offset = inoffset;

    gl_Position = ubuf.qt_Matrix * vertexCoord;
}
