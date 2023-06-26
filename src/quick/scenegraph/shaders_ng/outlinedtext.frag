// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#version 440

layout(location = 0) in vec2 sampleCoord;
layout(location = 1) in vec2 sCoordUp;
layout(location = 2) in vec2 sCoordDown;
layout(location = 3) in vec2 sCoordLeft;
layout(location = 4) in vec2 sCoordRight;

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
    float glyph = texture(_qt_texture, sampleCoord).r;
    float outline = clamp(clamp(texture(_qt_texture, sCoordUp).r +
                                texture(_qt_texture, sCoordDown).r +
                                texture(_qt_texture, sCoordLeft).r +
                                texture(_qt_texture, sCoordRight).r,
                                0.0, 1.0) - glyph,
                          0.0, 1.0);
     fragColor = outline * styleColor + step(1.0 - glyph, 1.0) * glyph * color;
}
