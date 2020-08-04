#version 150 core

in vec2 sampleCoord;

out vec4 fragColor;

uniform sampler2D _qt_texture;
uniform vec4 color;
uniform vec4 styleColor;
uniform float alphaMin;
uniform float alphaMax;
uniform float outlineAlphaMax0;
uniform float outlineAlphaMax1;

void main()
{
    float d = texture(_qt_texture, sampleCoord).r;
    float a = smoothstep(alphaMin, alphaMax, d);
    fragColor = step(1.0 - a, 1.0) * mix(styleColor, color, a)
        * smoothstep(outlineAlphaMax0, outlineAlphaMax1, d);
}
