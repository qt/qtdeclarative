#version 440

layout(location = 0) in vec2 sampleCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D _qt_texture;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    vec4 color; // only alpha is used, but must be vec4 due to layout compat
    vec2 textureScale;
    float dpr;
} ubuf;

void main()
{
    fragColor = texture(_qt_texture, sampleCoord) * ubuf.color.a;
}
