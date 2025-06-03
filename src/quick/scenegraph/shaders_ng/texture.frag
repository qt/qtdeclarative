// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#version 440

layout(location = 0) in vec2 qt_TexCoord;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float opacity;
} ubuf;

layout(binding = 1) uniform sampler2D qt_Texture;

void main()
{
    fragColor = texture(qt_Texture, qt_TexCoord) * ubuf.opacity;
}
