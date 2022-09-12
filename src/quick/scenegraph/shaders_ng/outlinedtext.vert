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
    mat4 projectionMatrix;
    vec4 color;
    vec2 textureScale;
    float dpr;
    // the above must stay compatible with textmask/8bittextmask
    vec4 styleColor;
    vec2 shift;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
     sampleCoord = tCoord * ubuf.textureScale;
     sCoordUp = (tCoord - vec2(0.0, -1.0)) * ubuf.textureScale;
     sCoordDown = (tCoord - vec2(0.0, 1.0)) * ubuf.textureScale;
     sCoordLeft = (tCoord - vec2(-1.0, 0.0)) * ubuf.textureScale;
     sCoordRight = (tCoord - vec2(1.0, 0.0)) * ubuf.textureScale;
     vec4 xformed = ubuf.modelViewMatrix * vCoord;
     gl_Position = ubuf.projectionMatrix * vec4(floor(xformed.xyz * ubuf.dpr + 0.5) / ubuf.dpr, xformed.w);
}
