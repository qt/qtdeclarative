#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D source;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 offset;
    vec2 delta;
} ubuf;

void main() {
    vec2 delta2 = vec2(ubuf.delta.x, -ubuf.delta.y);
    float shadow = 0.25 * (texture(source, qt_TexCoord0 - ubuf.offset + ubuf.delta).a
                         + texture(source, qt_TexCoord0 - ubuf.offset - ubuf.delta).a
                         + texture(source, qt_TexCoord0 - ubuf.offset + delta2).a
                         + texture(source, qt_TexCoord0 - ubuf.offset - delta2).a);
    vec4 color = texture(source, qt_TexCoord0);
    fragColor = mix(vec4(vec3(0.), 0.5 * shadow), color, color.a);
}
