#version 440

layout(location = 0) in float gradTabIndex;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D gradTabTexture;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    vec2 gradStart;
    vec2 gradEnd;
    float opacity;
} ubuf;

void main()
{
    fragColor = texture(gradTabTexture, vec2(gradTabIndex, 0.5)) * ubuf.opacity;
}
