#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D source;

void main() {
    lowp float r = mod(qt_TexCoord0.x * 10.0, 1.0);
    lowp float g = mod(qt_TexCoord0.y * 10.0, 1.0);
    lowp float b = qt_TexCoord0.x;
    fragColor = vec4(r, g, b, 1) * texture(source, qt_TexCoord0).a;
}
