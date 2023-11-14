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

#version 150 core

in vec2 sampleCoord;
in vec2 sCoordUp;
in vec2 sCoordDown;
in vec2 sCoordLeft;
in vec2 sCoordRight;

out vec4 fragColor;

uniform sampler2D _qt_texture;
uniform vec4 color;
uniform vec4 styleColor;

void main()
{
    float glyph = texture(_qt_texture, sampleCoord).r;
    float outline = clamp(clamp(texture(_qt_texture, sCoordUp).r +
                                texture(_qt_texture, sCoordDown).r +
                                texture(_qt_texture, sCoordLeft).r +
                                texture(_qt_texture, sCoordRight).r,
                                0.0, 1.0) - glyph,
                          0.0, 1.0);
    fragColor = outline * styleColor + step(1.0 -  glyph, 1.0) * glyph * color;
}
