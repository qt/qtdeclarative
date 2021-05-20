#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(binding = 1) uniform sampler2D source;

void main() {
    lowp vec4 c = texture(source, qt_TexCoord0);
    lowp float level = c.r * 0.3 + c.g * 0.59 + c.b * 0.11;

    fragColor = vec4(level, level, level, c.a);
}
