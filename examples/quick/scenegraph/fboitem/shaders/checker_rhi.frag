#version 440

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;

    vec4 color1;
    vec4 color2;
    vec2 pixelSize;
} ubuf;

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

void main()
{
    vec2 tc = sign(sin(3.14159265358979323846 * qt_TexCoord0 * ubuf.pixelSize));
    if (tc.x != tc.y)
        fragColor = ubuf.color1;
    else
        fragColor = ubuf.color2;
}
