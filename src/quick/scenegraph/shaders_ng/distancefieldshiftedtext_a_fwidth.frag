// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#version 440

layout(location = 0) in vec2 sampleCoord;
layout(location = 1) in vec2 shiftedSampleCoord;

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
    vec2 shift;
} ubuf;

void main()
{
    float distance = texture(_qt_texture, sampleCoord).a;
    float f = fwidth(distance);
    float a = smoothstep(0.5 - f, 0.5 + f, distance);

    float shiftedDistance = texture(_qt_texture, shiftedSampleCoord).a;
    float shiftedF = fwidth(shiftedDistance);
    float shiftedA = smoothstep(0.5 - shiftedF, 0.5 + shiftedF, shiftedDistance);

    vec4 shifted = ubuf.styleColor * shiftedA;
    fragColor = mix(shifted, ubuf.color, a);
}
