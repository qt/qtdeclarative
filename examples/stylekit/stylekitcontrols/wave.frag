// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D sourceItem;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;      // reserved
    float qt_Opacity;    // reserved
    float amplitude;
    float frequency;
    float time;
} args;

void main()
{
    vec2 p = sin(args.time + args.frequency * qt_TexCoord0);
    fragColor = texture(sourceItem, qt_TexCoord0 + args.amplitude * vec2(p.y, -p.x)) * args.qt_Opacity;
}
