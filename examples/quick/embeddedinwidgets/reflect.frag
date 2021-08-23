#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D source;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 sourceSize;
};

void main()
{
    vec2 tc = qt_TexCoord0 * vec2(1, -1) + vec2(0, 1);
    vec4 col = 0.25 * (texture(source, tc + sourceSize)
                       + texture(source, tc- sourceSize)
                       + texture(source, tc + sourceSize * vec2(1, -1))
                       + texture(source, tc + sourceSize * vec2(-1, 1)));
    fragColor = col * qt_Opacity * (1.0 - qt_TexCoord0.y) * 0.2;
}
