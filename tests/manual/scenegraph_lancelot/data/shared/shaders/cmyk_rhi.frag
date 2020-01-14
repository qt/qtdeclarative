#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D cyan;
layout(binding = 2) uniform sampler2D magenta;
layout(binding = 3) uniform sampler2D yellow;
layout(binding = 4) uniform sampler2D black;

void main() {
    vec2 t = qt_TexCoord0 * 3. - 1.;
    lowp float c = texture(cyan, t + vec2(.05, .09)).a;
    lowp float m = texture(magenta, t + vec2(.04, -.10)).a;
    lowp float y = texture(yellow, t + vec2(-.10, .01)).a;
    lowp float k = texture(black, t).a;
    fragColor = 1. - vec4(c + k, m + k, y + k, 0.);
}
