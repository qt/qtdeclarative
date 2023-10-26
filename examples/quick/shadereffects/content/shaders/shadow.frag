// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D source;
layout(binding = 2) uniform sampler2D shadow;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float darkness;
    vec2 delta;
} ubuf;

void main()
{
    vec4 fg = texture(source, qt_TexCoord0);
    vec4 bg = texture(shadow, qt_TexCoord0 + ubuf.delta);
    fragColor = (fg + vec4(0., 0., 0., ubuf.darkness * bg.a) * (1. - fg.a)) * ubuf.qt_Opacity;
}
