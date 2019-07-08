#version 440

layout(location = 0) in vec2 sampleCoord;
layout(location = 1) in vec3 sampleFarLeft;
layout(location = 2) in vec3 sampleNearLeft;
layout(location = 3) in vec3 sampleNearRight;
layout(location = 4) in vec3 sampleFarRight;

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
    vec4 n;
    n.x = textureProj(_qt_texture, sampleFarLeft).r;
    n.y = textureProj(_qt_texture, sampleNearLeft).r;
    float c = texture(_qt_texture, sampleCoord).r;
    n.z = textureProj(_qt_texture, sampleNearRight).r;
    n.w = textureProj(_qt_texture, sampleFarRight).r;

    vec2 d = min(abs(n.yw - n.xz) * 2., 0.67);
    vec2 lo = mix(vec2(ubuf.alphaMin), vec2(0.5), d);
    vec2 hi = mix(vec2(ubuf.alphaMax), vec2(0.5), d);
    n = smoothstep(lo.xxyy, hi.xxyy, n);
    c = smoothstep(lo.x + lo.y, hi.x + hi.y, 2. * c);

    fragColor = vec4(0.333 * (n.xyz + n.yzw + c), c) * ubuf.color.w;
}
