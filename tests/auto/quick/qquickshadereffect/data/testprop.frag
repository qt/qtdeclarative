#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D source;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec4 aVec4;
    vec3 aVec3;
    vec2 aVec2;
    float aFloat;
};

void main()
{
    vec4 c = texture(source, qt_TexCoord0);
    vec4 v = aVec4;
    v.rgb += aVec3;
    v.rg += aVec2;
    v.r += aFloat;
    c *= v;
    if (c.a == 0.0)
        c.a = 1.0;
    fragColor = c * qt_Opacity;
}
