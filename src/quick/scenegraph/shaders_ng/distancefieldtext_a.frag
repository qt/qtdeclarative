#version 440

layout(location = 0) in vec2 sampleCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D _qt_texture;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    vec2 textureScale;
    vec4 color;
    float alphaMin;
    float alphaMax;
} ubuf;

void main()
{
    fragColor = ubuf.color * smoothstep(ubuf.alphaMin, ubuf.alphaMax,
                                        texture(_qt_texture, sampleCoord).a);
}
