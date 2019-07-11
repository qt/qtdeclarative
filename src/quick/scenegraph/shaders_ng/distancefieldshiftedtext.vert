#version 440

layout(location = 0) in vec4 vCoord;
layout(location = 1) in vec2 tCoord;

layout(location = 0) out vec2 sampleCoord;
layout(location = 1) out vec2 shiftedSampleCoord;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    vec2 textureScale;
    vec4 color;
    float alphaMin;
    float alphaMax;
    // up to this point it must match distancefieldtext
    vec4 styleColor;
    vec2 shift;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
     sampleCoord = tCoord * ubuf.textureScale;
     shiftedSampleCoord = (tCoord - ubuf.shift) * ubuf.textureScale;
     gl_Position = ubuf.matrix * vCoord;
}
