#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D frontSource;
layout(binding = 1) uniform sampler2D backSource;

void main() {
    fragColor = gl_FrontFacing
                 ? texture(frontSource, qt_TexCoord0)
                 : texture(backSource, qt_TexCoord0);
}
