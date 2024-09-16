// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#version 440

layout(location = 0) in vec2 sampleCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D _qt_texture;

layout(std140, binding = 0) uniform buf {
    mat4 modelViewMatrix;
#if QSHADER_VIEW_COUNT >= 2
    mat4 projectionMatrix[QSHADER_VIEW_COUNT];
#else
    mat4 projectionMatrix;
#endif
    vec2 textureScale;
    float dpr;
    vec4 color;
};

void main()
{
    vec4 glyph = texture(_qt_texture, sampleCoord);
    fragColor = glyph * color.a;
}
