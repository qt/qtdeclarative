// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#version 440

layout(location = 0) in vec2 pos;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    mat4 rotation;
    vec4 color;
    float pattern;
    int projection;
} ubuf;

void main(void)
{
    vec4 c = ubuf.color;
    c.xyz += pow(max(sin(pos.x + pos.y), 0.0), 2.0) * ubuf.pattern * 0.25;
    fragColor = c;
}
