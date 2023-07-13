#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float textureWidth;
    float textureHeight;
};
layout(binding = 1) uniform sampler2D src;


void main(void)
{

    float xPixelIndex = (round((textureWidth - 1.) * qt_TexCoord0.x) + 0.5) / textureWidth;
    float yPixelIndex = (round((textureHeight - 1.) * qt_TexCoord0.y) + 0.5) / textureHeight;

    fragColor = texture(src, vec2(xPixelIndex, yPixelIndex)) * qt_Opacity;
}
