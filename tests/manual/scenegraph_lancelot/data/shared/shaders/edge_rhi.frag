#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D source;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 textureSize;
    vec4 color;
} ubuf;

void main() {
    vec2 dx = vec2(0.5 / ubuf.textureSize.x, 0.);
    vec2 dy = vec2(0., 0.5 / ubuf.textureSize.y);
    fragColor = ubuf.color * 0.25
                * (texture(source, qt_TexCoord0 + dx + dy).a
                + texture(source, qt_TexCoord0 + dx - dy).a
                + texture(source, qt_TexCoord0 - dx + dy).a
                + texture(source, qt_TexCoord0 - dx - dy).a);
}
