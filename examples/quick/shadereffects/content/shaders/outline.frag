#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D source;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 delta;
} ubuf;

void main()
{
    vec4 tl = texture(source, qt_TexCoord0 - ubuf.delta);
    vec4 tr = texture(source, qt_TexCoord0 + vec2(ubuf.delta.x, -ubuf.delta.y));
    vec4 bl = texture(source, qt_TexCoord0 - vec2(ubuf.delta.x, -ubuf.delta.y));
    vec4 br = texture(source, qt_TexCoord0 + ubuf.delta);
    vec4 gx = (tl + bl) - (tr + br);
    vec4 gy = (tl + tr) - (bl + br);
    fragColor.xyz = vec3(0.);
    fragColor.w = clamp(dot(sqrt(gx * gx + gy * gy), vec4(1.)), 0., 1.) * ubuf.qt_Opacity;
}
