// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#version 440

layout(std140, binding = 0) uniform buf {
    // built-in Qt data
    mat4 qt_Matrix;
    float qt_Opacity;
    // custom data
    vec4 color1;
    vec4 color2;
    vec2 pixelSize;
};

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

void main()
{
    vec2 tc = sign(sin(3.14159265358979323846 * qt_TexCoord0 * pixelSize));
    if (tc.x != tc.y)
        fragColor = color1;
    else
        fragColor = color2;
}
