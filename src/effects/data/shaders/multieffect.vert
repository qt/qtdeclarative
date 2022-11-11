#version 440

layout(location = 0) in vec4 qt_Vertex;
layout(location = 1) in vec2 qt_MultiTexCoord0;
layout(location = 0) out vec2 texCoord;
#if defined(SHADOW)
layout(location = 1) out vec2 shadowTexCoord;
#endif

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;

    float shadowScale;
    vec2 shadowOffset;
    vec2 centerOffset;

    float contrast;
    float brightness;
    float saturation;
    vec4 colorizeColor;
    vec4 blurWeight1;
    vec2 blurWeight2;
    vec4 mask;
    float maskInverted;
    vec4 shadowColor;
    vec4 shadowBlurWeight1;
    vec2 shadowBlurWeight2;
};

void main() {
    texCoord = qt_MultiTexCoord0;
#if defined(SHADOW)
    shadowTexCoord = qt_MultiTexCoord0 - shadowOffset;
    shadowTexCoord = (shadowTexCoord * shadowScale) + centerOffset;
#endif
    gl_Position = qt_Matrix * qt_Vertex;
}
