#version 150

uniform float qt_Opacity;
uniform sampler2D source;
uniform sampler2D maskSource;

in vec2 qt_TexCoord0;
out vec4 fragColor;

void main()
{
    fragColor = texture(source, qt_TexCoord0.st) * (texture(maskSource, qt_TexCoord0.st).a) * qt_Opacity;
}
