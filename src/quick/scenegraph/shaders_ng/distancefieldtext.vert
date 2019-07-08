#version 440

layout(location = 0) in vec4 vCoord;
layout(location = 1) in vec2 tCoord;

layout(location = 0) out vec2 sampleCoord;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    vec2 textureScale;
    vec4 color;
    float alphaMin;
    float alphaMax;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
     sampleCoord = tCoord * ubuf.textureScale;
     gl_Position = ubuf.matrix * vCoord;
}
