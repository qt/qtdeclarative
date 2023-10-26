// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#version 440

layout(location = 0) in vec2 texCoord;
#if defined(SHADOW)
layout(location = 1) in vec2 shadowTexCoord;
#endif
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;

    float shadowScale;
    vec2 shadowOffset;
    vec2 centerOffset;

    float contrast;
    float brightness;
    float saturation;
    vec4 colorizationColor;
    vec4 blurWeight1;
    vec2 blurWeight2;
    vec4 mask;
    float maskInverted;
    vec4 shadowColor;
    vec4 shadowBlurWeight1;
    vec2 shadowBlurWeight2;
};

layout(binding = 1) uniform sampler2D src;

#if defined(BLUR) || defined(SHADOW)
#if defined(BL1)
layout(binding = 2) uniform sampler2D blurSrc1;
layout(binding = 3) uniform sampler2D blurSrc2;
layout(binding = 4) uniform sampler2D blurSrc3;
#endif
#if defined(BL2)
layout(binding = 5) uniform sampler2D blurSrc4;
#endif
#if defined(BL3)
layout(binding = 6) uniform sampler2D blurSrc5;
#endif
#endif // BLUR || SHADOW

#if defined(MASK)
#if defined(BL3)
layout(binding = 7) uniform sampler2D maskSrc;
#elif defined(BL2)
layout(binding = 6) uniform sampler2D maskSrc;
#elif defined(BL1)
layout(binding = 5) uniform sampler2D maskSrc;
#else // BL0 or no blur or shadow
layout(binding = 2) uniform sampler2D maskSrc;
#endif
#endif

void main() {

#if !defined(BLUR)
    vec4 color = texture(src, texCoord);
#else // BLUR
    vec4 color = texture(src, texCoord) * blurWeight1[0];
#if defined(BL1)
    color += texture(blurSrc1, texCoord) * blurWeight1[1];
    color += texture(blurSrc2, texCoord) * blurWeight1[2];
    color += texture(blurSrc3, texCoord) * blurWeight1[3];
#endif
#if defined(BL2)
    color += texture(blurSrc4, texCoord) * blurWeight2[0];
#endif
#if defined(BL3)
    color += texture(blurSrc5, texCoord) * blurWeight2[1];
#endif
#endif // BLUR

    // contrast, brightness, saturation and colorization
    color.rgb = (color.rgb - 0.5 * color.a) * (1.0 + contrast) + 0.5 * color.a;
    color.rgb += brightness * color.a;
    float gray = dot(color.rgb, vec3(0.299, 0.587, 0.114));
    color.rgb = gray * colorizationColor.rgb * colorizationColor.a + color.rgb * (1.0 - colorizationColor.a);
    color.rgb = mix(vec3(gray), color.rgb, 1.0 + saturation);

#if defined(SHADOW)
#if defined(BL0)
    float shadow = texture(src, shadowTexCoord).a;
#endif
#if defined(BL1)
    float shadow = texture(src, shadowTexCoord).a * shadowBlurWeight1[0];
    shadow += texture(blurSrc1, shadowTexCoord).a * shadowBlurWeight1[1];
    shadow += texture(blurSrc2, shadowTexCoord).a * shadowBlurWeight1[2];
    shadow += texture(blurSrc3, shadowTexCoord).a * shadowBlurWeight1[3];
#endif
#if defined(BL2)
    shadow += texture(blurSrc4, shadowTexCoord).a * shadowBlurWeight2[0];
#endif
#if defined(BL3)
    shadow += texture(blurSrc5, shadowTexCoord).a * shadowBlurWeight2[1];
#endif
    shadow *= shadowColor.a;
    float aa = (1.0 - color.a) * (1.0 - shadow);
    color.rgb = mix(shadowColor.rgb * shadow, color.rgb, color.a + aa);
    color.a = 1.0 - aa;
#endif // SHADOW

#if defined(MASK)
    float alphaMask = texture(maskSrc, texCoord).a;
    float m1 = smoothstep(mask[0], mask[1], alphaMask);
    float m2 = smoothstep(mask[2], mask[3], (1.0 - alphaMask));
    float mm = m1 * m2;
    color *= (1.0 - maskInverted) * mm + maskInverted * (1.0 - mm);
#endif // MASK

    fragColor = color * qt_Opacity;
}
