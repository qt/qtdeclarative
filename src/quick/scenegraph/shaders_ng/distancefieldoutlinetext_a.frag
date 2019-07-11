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
    // up to this point it must match distancefieldtext
    vec4 styleColor;
    float outlineAlphaMax0;
    float outlineAlphaMax1;
} ubuf;

void main()
{
    float d = texture(_qt_texture, sampleCoord).a;
    fragColor = mix(ubuf.styleColor, ubuf.color, smoothstep(ubuf.alphaMin, ubuf.alphaMax, d))
            * smoothstep(ubuf.outlineAlphaMax0, ubuf.outlineAlphaMax1, d);
}
