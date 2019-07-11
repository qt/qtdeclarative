#version 440

layout(location = 0) in vec2 coord;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D gradTabTexture;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    vec2 translationPoint;
    vec2 focalToCenter;
    float centerRadius;
    float focalRadius;
    float opacity;
} ubuf;

void main()
{
    float rd = ubuf.centerRadius - ubuf.focalRadius;
    float b = 2.0 * (rd * ubuf.focalRadius + dot(coord, ubuf.focalToCenter));
    float fmp2_m_radius2 = -ubuf.focalToCenter.x * ubuf.focalToCenter.x - ubuf.focalToCenter.y * ubuf.focalToCenter.y + rd * rd;
    float inverse_2_fmp2_m_radius2 = 1.0 / (2.0 * fmp2_m_radius2);
    float det = b * b - 4.0 * fmp2_m_radius2 * ((ubuf.focalRadius * ubuf.focalRadius) - dot(coord, coord));
    vec4 result = vec4(0.0);
    if (det >= 0.0) {
        float detSqrt = sqrt(det);
        float w = max((-b - detSqrt) * inverse_2_fmp2_m_radius2, (-b + detSqrt) * inverse_2_fmp2_m_radius2);
        if (ubuf.focalRadius + w * (ubuf.centerRadius - ubuf.focalRadius) >= 0.0)
            result = texture(gradTabTexture, vec2(w, 0.5)) * ubuf.opacity;
    }
    fragColor = result;
}
