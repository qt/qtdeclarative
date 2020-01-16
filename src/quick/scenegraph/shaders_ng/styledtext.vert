#version 440

layout(location = 0) in vec4 vCoord;
layout(location = 1) in vec2 tCoord;

layout(location = 0) out vec2 sampleCoord;
layout(location = 1) out vec2 shiftedSampleCoord;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
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
     shiftedSampleCoord = (tCoord - ubuf.shift) * ubuf.textureScale;
     vec3 dprSnapPos = floor(vCoord.xyz * ubuf.dpr + 0.5) / ubuf.dpr;
     gl_Position = ubuf.matrix * vec4(dprSnapPos, vCoord.w);
}
