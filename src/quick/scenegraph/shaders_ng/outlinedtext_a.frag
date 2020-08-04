#version 440

layout(location = 0) in vec2 sampleCoord;
layout(location = 1) in vec2 sCoordUp;
layout(location = 2) in vec2 sCoordDown;
layout(location = 3) in vec2 sCoordLeft;
layout(location = 4) in vec2 sCoordRight;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D _qt_texture;

layout(std140, binding = 0) uniform buf {
    // must match styledtext
    mat4 matrix;
    vec4 color;
    vec2 textureScale;
    float dpr;
    vec4 styleColor;
    vec2 shift;
} ubuf;

void main()
{
    float glyph = texture(_qt_texture, sampleCoord).a; // take .a instead of .r
    float outline = clamp(clamp(texture(_qt_texture, sCoordUp).a +
                                texture(_qt_texture, sCoordDown).a +
                                texture(_qt_texture, sCoordLeft).a +
                                texture(_qt_texture, sCoordRight).a,
                                0.0, 1.0) - glyph,
                          0.0, 1.0);
    fragColor = outline * ubuf.styleColor + step(1.0 - glyph, 1.0) * glyph * ubuf.color;
}
