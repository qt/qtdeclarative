#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D source;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec4 tint;
} ubuf;

void main()
{
    vec4 c = texture(source, qt_TexCoord0);
    float lo = min(min(c.x, c.y), c.z);
    float hi = max(max(c.x, c.y), c.z);
    fragColor = ubuf.qt_Opacity * vec4(mix(vec3(lo), vec3(hi), ubuf.tint.xyz), c.w);
}
