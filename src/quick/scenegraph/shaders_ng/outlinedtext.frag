/****************************************************************************
**
** Copyright (C) 2023 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
    mat4 projectionMatrix;
    vec4 color;
    vec2 textureScale;
    float dpr;
    // the above must stay compatible with textmask/8bittextmask
    vec4 styleColor;
    vec2 shift;
} ubuf;

void main()
{
    float glyph = texture(_qt_texture, sampleCoord).r;
    float outline = clamp(clamp(texture(_qt_texture, sCoordUp).r +
                                texture(_qt_texture, sCoordDown).r +
                                texture(_qt_texture, sCoordLeft).r +
                                texture(_qt_texture, sCoordRight).r,
                                0.0, 1.0) - glyph,
                          0.0, 1.0);
     fragColor = outline * ubuf.styleColor + step(1.0 - glyph, 1.0) * glyph * ubuf.color;
}
