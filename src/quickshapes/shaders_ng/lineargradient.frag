// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
