#version 440

layout(location = 0) in vec3 sampleNearLeft;
layout(location = 1) in vec3 sampleNearRight;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D _qt_texture;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    vec2 textureScale;
    vec4 color;
    float alphaMin;
    float alphaMax;
    // up to this point it must match distancefieldtext
    float fontScale;
    vec4 vecDelta;
} ubuf;

void main()
{
    vec2 n;
    n.x = textureProj(_qt_texture, sampleNearLeft).r;
    n.y = textureProj(_qt_texture, sampleNearRight).r;
    n = smoothstep(ubuf.alphaMin, ubuf.alphaMax, n);
    float c = 0.5 * (n.x + n.y);
    fragColor = vec4(n.x, c, n.y, c) * ubuf.color.w;
}
