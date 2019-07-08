#version 440

layout(location = 0) in vec2 sampleCoord;
layout(location = 1) in vec2 shiftedSampleCoord;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D _qt_texture;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    vec4 color;
    vec2 textureScale;
    float dpr;
    // the above must stay compatible with textmask/8bittextmask
    vec4 styleColor;
    vec2 shift;
} ubuf;

void main()
{
    float glyph = texture(_qt_texture, sampleCoord).r;
    float style = clamp(texture(_qt_texture, shiftedSampleCoord).r - glyph,
                        0.0, 1.0);
    fragColor = style * ubuf.styleColor + glyph * ubuf.color;
}
