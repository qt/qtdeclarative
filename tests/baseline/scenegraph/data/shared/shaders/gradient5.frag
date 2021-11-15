#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D source;

void main() {
    fragColor = vec4(qt_TexCoord0.x, 1, 0, 1) * texture(source, qt_TexCoord0).a;
}
