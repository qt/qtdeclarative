// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#version 440

layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 vShadeCoord;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    vec4 color;
    vec2 textureSize;
    float qt_Opacity;
};

layout(binding = 1) uniform sampler2D qt_Texture;

#define PI 3.14159265358979323846

void main()
{
    float shade = texture(qt_Texture, vTexCoord).r * 0.05 - length(vec2(0.5, 0.4) - vShadeCoord) * 0.3;
    vec4 c = vec4(color.xyz + shade, color.w);
    fragColor = c * qt_Opacity;
}
