#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D source;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
};

void main()
{
    vec4 c = texture(source, qt_TexCoord0);
    if (c.a == 0.0)
        c = vec4(0, 0, 1, 1);
    fragColor = c * qt_Opacity;
}
