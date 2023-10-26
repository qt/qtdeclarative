// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#version 440

layout(location = 0) in float vT;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    vec4 color;
    float qt_Opacity;
    float size;
    float spread;
};

#define PI 3.14159265358979323846

void main(void)
{
    float tt = smoothstep(spread, 1.0, sin(vT * PI));
    fragColor = color * qt_Opacity * tt;
}
