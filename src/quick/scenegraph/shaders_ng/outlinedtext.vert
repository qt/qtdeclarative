#version 440

layout(location = 0) in vec4 vCoord;
layout(location = 1) in vec2 tCoord;

layout(location = 0) out vec2 sampleCoord;
layout(location = 1) out vec2 sCoordUp;
layout(location = 2) out vec2 sCoordDown;
layout(location = 3) out vec2 sCoordLeft;
layout(location = 4) out vec2 sCoordRight;

layout(std140, binding = 0) uniform buf {
    mat4 modelViewMatrix;
#if QSHADER_VIEW_COUNT >= 2
    mat4 projectionMatrix[QSHADER_VIEW_COUNT];
#else
    mat4 projectionMatrix;
#endif
    vec2 textureScale;
    float dpr;
    vec4 color;
    // the above must stay compatible with textmask/8bittextmask
    vec4 styleColor;
    vec2 shift;
};

void main()
{
     sampleCoord = tCoord * textureScale;
     sCoordUp = (tCoord - vec2(0.0, -1.0)) * textureScale;
     sCoordDown = (tCoord - vec2(0.0, 1.0)) * textureScale;
     sCoordLeft = (tCoord - vec2(-1.0, 0.0)) * textureScale;
     sCoordRight = (tCoord - vec2(1.0, 0.0)) * textureScale;
     vec4 xformed = modelViewMatrix * vCoord;
#if QSHADER_VIEW_COUNT >= 2
     gl_Position = projectionMatrix[gl_ViewIndex] * vec4(floor(xformed.xyz * dpr + 0.5) / dpr, xformed.w);
#else
     gl_Position = projectionMatrix * vec4(floor(xformed.xyz * dpr + 0.5) / dpr, xformed.w);
#endif
}
