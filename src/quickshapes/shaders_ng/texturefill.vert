// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#version 440

layout(location = 0) in vec4 vertexCoord;
layout(location = 1) in vec4 vertexColor;

layout(location = 0) out vec2 textureCoord;

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
    vec2 xformed = (ubuf.fillMatrix * vertexCoord).xy;
    textureCoord = vec2(xformed.x / ubuf.boundsSize.x, xformed.y / ubuf.boundsSize.y);
#if QSHADER_VIEW_COUNT >= 2
    gl_Position = ubuf.qt_Matrix[gl_ViewIndex] * vertexCoord;
#else
    gl_Position = ubuf.qt_Matrix * vertexCoord;
#endif
}
