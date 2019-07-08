#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D source;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float amplitude;
    float frequency;
    float time;
} ubuf;

void main()
{
    vec2 p = sin(ubuf.time + ubuf.frequency * qt_TexCoord0);
    fragColor = texture(source, qt_TexCoord0 + ubuf.amplitude * vec2(p.y, -p.x)) * ubuf.qt_Opacity;
}
