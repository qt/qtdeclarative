#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

void main() {
    lowp float u = qt_TexCoord0.x;
    lowp float v = qt_TexCoord0.y;
    fragColor = vec4(u*v, v*v, v, v);
}
