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
} ubuf;

void main()
{
    float distance = texture(_qt_texture, sampleCoord).a;
    float f = fwidth(distance);
    fragColor = ubuf.color * smoothstep(max(0.0, 0.5 - f), min(1.0, 0.5 + f), distance);
}
