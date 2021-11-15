#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D maskSource;
layout(binding = 2) uniform sampler2D colorSource;

void main() {
    fragColor = texture(maskSource, qt_TexCoord0).a * texture(colorSource, qt_TexCoord0.yx);
}
