// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#version 440

layout(location = 0) in vec2 sampleCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D _qt_texture;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    vec2 textureScale;
    vec4 color;
    float alphaMin;
    float alphaMax;
    // up to this point it must match distancefieldtext
    vec4 styleColor;
    float outlineAlphaMax0;
    float outlineAlphaMax1;
} ubuf;

void main()
{
    float distance = texture(_qt_texture, sampleCoord).r;
    float f = fwidth(distance);

    // The outlineLimit is based on font size, but scales with the transform, so
    // we can calculate it from the outline span.
    float outlineLimit = (ubuf.outlineAlphaMax1 - ubuf.outlineAlphaMax0) / 2.0 + ubuf.outlineAlphaMax0;

    float a = smoothstep(max(0.0, 0.5 - f), min(1.0, 0.5 + f), distance);
    fragColor = step(1.0 - a, 1.0) * mix(ubuf.styleColor, ubuf.color, a) * smoothstep(max(0.0, outlineLimit - f), min(outlineLimit + f, 0.5 - f), distance);
}
