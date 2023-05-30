#version 440

layout(location = 0) in vec3 qt_TexCoord;
layout(location = 1) in vec4 debugColor;

#if defined(LINEARGRADIENT)
layout(location = 2) in float gradTabIndex;
#elif defined(RADIALGRADIENT) || defined(CONICALGRADIENT)
layout(location = 2) in vec2 coord;
#endif

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;

#if defined(STROKE)
    vec4 strokeColor;
    float strokeWidth;
    float reserved1;
    float reserved2;
    float reserved3;
#endif

#if defined(LINEARGRADIENT)
    vec2 gradientStart;
    vec2 gradientEnd;
#elif defined(RADIALGRADIENT)
    vec2 translationPoint;
    vec2 focalToCenter;
    float centerRadius;
    float focalRadius;
#elif defined(CONICALGRADIENT)
    vec2 translationPoint;
    float angle;
#else
    vec4 color;
#endif
} ubuf;

#define INVERSE_2PI 0.1591549430918953358

#if defined(LINEARGRADIENT) || defined(RADIALGRADIENT) || defined(CONICALGRADIENT)
layout(binding = 1) uniform sampler2D gradTabTexture;
#endif

vec4 baseColor()
{
#if defined(LINEARGRADIENT)
    return texture(gradTabTexture, vec2(gradTabIndex, 0.5));
#elif defined(RADIALGRADIENT)
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
            result = texture(gradTabTexture, vec2(w, 0.5));
    }

    return result;
#elif defined(CONICALGRADIENT)
    float t;
    if (abs(coord.y) == abs(coord.x))
        t = (atan(-coord.y + 0.002, coord.x) + ubuf.angle) * INVERSE_2PI;
    else
        t = (atan(-coord.y, coord.x) + ubuf.angle) * INVERSE_2PI;
    return texture(gradTabTexture, vec2(t - floor(t), 0.5));
#else
    return vec4(ubuf.color.rgb, 1.0) * ubuf.color.a;
#endif
}

void main()
{
    float f = qt_TexCoord.z * (qt_TexCoord.x * qt_TexCoord.x - qt_TexCoord.y) // curve
              + (1.0 - abs(qt_TexCoord.z)) * (qt_TexCoord.x - qt_TexCoord.y); // line

#if defined(STROKE)
    float _ddx = dFdx(f);
    float _ddy = dFdy(f);
    float df = length(vec2(_ddx, _ddy));
    float distance = (f / df); // distance from centre of fragment to line

    float halfStrokeWidth = ubuf.strokeWidth / 2.0;

    // calculate stroke
    float strokeCoverage = 1.0 - clamp(0.5 + abs(distance) - halfStrokeWidth, 0.0, 1.0);
    vec4 stroke = ubuf.strokeColor * strokeCoverage;

    float fillCoverage = clamp(0.5 + f / df, 0.0, 1.0);
    vec4 fill = baseColor() * fillCoverage;

    vec4 combined = fill * (1.0 - stroke.a) +  stroke * stroke.a;

    // finally mix in debug
    fragColor = mix(combined, vec4(debugColor.rgb, 1.0), debugColor.a);

    // TODO: support both outline and fill
#else
    float df = fwidth(f);
    fragColor = mix(baseColor() * clamp(0.5 + f / df, 0.0, 1.0), vec4(debugColor.rgb, 1.0), debugColor.a);
#endif
}
