// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#version 440

layout(location = 0) in vec2 sampleCoord;
layout(location = 1) in vec2 shiftedSampleCoord;

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
    // the above must stay compatible with textmask/8bittextmask
    vec4 styleColor;
    vec2 shift;
};

void main()
{
    float glyph = texture(_qt_texture, sampleCoord).a; // take .a instead of .r
    float style = clamp(texture(_qt_texture, shiftedSampleCoord).a - glyph,
                        0.0, 1.0);
    fragColor = style * styleColor + glyph * color;
}
