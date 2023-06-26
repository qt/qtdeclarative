#version 440

layout(location = 0) in vec4 vCoord;
layout(location = 1) in vec2 tCoord;

layout(location = 0) out vec2 sampleCoord;

layout(std140, binding = 0) uniform buf {
#if QSHADER_VIEW_COUNT >= 2
    mat4 matrix[QSHADER_VIEW_COUNT];
#else
    mat4 matrix;
#endif
    vec2 textureScale;
    vec4 color;
    float alphaMin;
    float alphaMax;
};

void main()
{
    sampleCoord = tCoord * textureScale;
#if QSHADER_VIEW_COUNT >= 2
    gl_Position = matrix[gl_ViewIndex] * vCoord;
#else
    gl_Position = matrix * vCoord;
#endif
}
