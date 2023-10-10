// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#version 440

layout(location = 0) in vec4 qt_TexCoord;
layout(location = 1) in vec4 gradient;

#if defined(LINEARGRADIENT)
layout(location = 2) in float gradTabIndex;
#elif defined(RADIALGRADIENT) || defined(CONICALGRADIENT)
layout(location = 2) in vec2 coord;
#endif


layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float matrixScale;
    float opacity;
    float debug;
    float reserved3;

#if defined(STROKE)
    vec4 strokeColor;
    float strokeWidth;
    float reserved4;
    float reserved5;
    float reserved6;
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

#if defined(USE_DERIVATIVES)
    float _ddx = dFdx(f);
    float _ddy = dFdy(f);
    float df = length(vec2(_ddx, _ddy));
#else
    // We calculate the partial derivatives for f'(x, y) based on knowing the partial derivatives
    // for the texture coordinates (u, v).
    // So for curves:
    //     f(x,y) = u(x, y) * u(x, y) - v(x, y)
    //     f(x,y) = p(u(x,y)) - v(x, y) where p(u) = u^2
    // So  f'(x, y) = p'(u(x, y)) * u'(x, y) - v'(x, y)
    // (by chain rule and sum rule)
    // f'(x, y) = 2 * u(x, y) * u'(x, y) - v'(x, y)
    // And so:
    // df/dx = 2 * u(x, y) * du/dx - dv/dx
    // df/dy = 2 * u(x, y) * du/dy - dv/dy
    //
    // and similarly for straight lines:
    // f(x, y) = u(x, y) - v(x, y)
    // f'(x, y) = dudx - dvdx

    float dudx = gradient.x;
    float dvdx = gradient.y;
    float dudy = gradient.z;
    float dvdy = gradient.w;

    // Test with analytic derivatives
//    dudx = dFdx(qt_TexCoord.x);
//    dvdx = dFdx(qt_TexCoord.y);
//    dudy = dFdy(qt_TexCoord.x);
//    dvdy = dFdy(qt_TexCoord.y);

    float dfx_curve = 2.0f * qt_TexCoord.x * dudx - dvdx;
    float dfy_curve = 2.0f * qt_TexCoord.x * dudy - dvdy;

    float dfx_line = dudx - dvdx;
    float dfy_line = dudy - dvdy;

    float dfx = qt_TexCoord.z * dfx_curve + (1.0 - abs(qt_TexCoord.z)) * dfx_line;
    float dfy = qt_TexCoord.z * dfy_curve + (1.0 - abs(qt_TexCoord.z)) * dfy_line;
    float df = length(vec2(dfx, dfy));
#endif

    float isLine = 1.0 - abs(qt_TexCoord.z);
    float isCurve = 1.0 - isLine;
    float debugR = isCurve * min(1.0, 1.0 - qt_TexCoord.z);
    float debugG = isLine;
    float debugB = isCurve * min(1.0, 1.0 - qt_TexCoord.z * -1.0) + debugG;
    vec3 debugColor = vec3(debugR, debugG, debugB);

#if defined(STROKE)
    float distance = (f / df); // distance from centre of fragment to line

    float halfStrokeWidth = ubuf.strokeWidth / 2.0;

    // calculate stroke
    float strokeCoverage = 1.0 - clamp(0.5 + abs(distance) - halfStrokeWidth, 0.0, 1.0);
    vec4 stroke = ubuf.strokeColor * strokeCoverage;

    float fillCoverage = clamp(0.5 + f / df, 0.0, 1.0);
    vec4 fill = baseColor() * fillCoverage;

    vec4 combined = fill * (1.0 - stroke.a) +  stroke * stroke.a;

    // finally mix in debug
    fragColor = mix(combined, vec4(debugColor, 1.0), ubuf.debug) * ubuf.opacity;
#else
    // Special case: mask out concave curve in "negative space".
    int specialCaseMask = 1 - int(qt_TexCoord.w != 0.0) * (int(qt_TexCoord.x < 0.0) +  int(qt_TexCoord.x > 1.0));
    float fillCoverage = clamp(0.5 + f / df, 0.0, 1.0) * float(specialCaseMask);

    fragColor = mix(baseColor() * fillCoverage, vec4(debugColor, 1.0), ubuf.debug) * ubuf.opacity;
#endif
}
