// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#version 440

layout(location = 0) in vec2 textureCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D sourceTexture;

layout(std140, binding = 0) uniform buf {
#if QSHADER_VIEW_COUNT >= 2
    mat4 qt_Matrix[QSHADER_VIEW_COUNT];
#else
    mat4 qt_Matrix;
#endif
    mat4 fillMatrix;
    vec2 boundsSize;
    float qt_Opacity;
} ubuf;

void main()
{
    fragColor = texture(sourceTexture, textureCoord) * ubuf.qt_Opacity;
}
