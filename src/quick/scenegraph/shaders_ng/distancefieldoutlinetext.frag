// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#version 440

layout(location = 0) in vec2 sampleCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D _qt_texture;

layout(std140, binding = 0) uniform buf {
#if QSHADER_VIEW_COUNT >= 2
    mat4 matrix[QSHADER_VIEW_COUNT];
#else
    mat4 matrix;
#endif
    vec2 textureScale;
    vec4 color;
    float alphaMin;
    float alphaMax;
    // up to this point it must match distancefieldtext
    vec4 styleColor;
    float outlineAlphaMax0;
    float outlineAlphaMax1;
};

void main()
{
    float d = texture(_qt_texture, sampleCoord).r;
    float a = smoothstep(alphaMin, alphaMax, d);
    fragColor = step(1.0 - a, 1.0) * mix(styleColor, color, a)
            * smoothstep(outlineAlphaMax0, outlineAlphaMax1, d);
}
