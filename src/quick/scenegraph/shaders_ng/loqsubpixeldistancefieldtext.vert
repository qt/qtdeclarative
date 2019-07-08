#version 440

layout(location = 0) in vec4 vCoord;
layout(location = 1) in vec2 tCoord;

layout(location = 0) out vec3 sampleNearLeft;
layout(location = 1) out vec3 sampleNearRight;

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

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    vec2 sampleCoord = tCoord * ubuf.textureScale;
    gl_Position = ubuf.matrix * vCoord;

    // Calculate neighbor pixel position in item space.
    vec3 wDelta = gl_Position.w * ubuf.vecDelta.xyw;
    vec3 nearLeft = vCoord.xyw - 0.25 * wDelta;
    vec3 nearRight = vCoord.xyw + 0.25 * wDelta;

    // Calculate neighbor texture coordinate.
    vec2 scale = ubuf.textureScale / ubuf.fontScale;
    vec2 base = sampleCoord - scale * vCoord.xy;
    sampleNearLeft = vec3(base * nearLeft.z + scale * nearLeft.xy, nearLeft.z);
    sampleNearRight = vec3(base * nearRight.z + scale * nearRight.xy, nearRight.z);
}
