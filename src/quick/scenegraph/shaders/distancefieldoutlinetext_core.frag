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

out vec4 fragColor;

uniform sampler2D _qt_texture;
uniform vec4 color;
uniform vec4 styleColor;
uniform float alphaMin;
uniform float alphaMax;
uniform float outlineAlphaMax0;
uniform float outlineAlphaMax1;

void main()
{
    float d = texture(_qt_texture, sampleCoord).r;
    float a = smoothstep(alphaMin, alphaMax, d);
    fragColor = step(1.0 - a, 1.0) * mix(styleColor, color, a)
        * smoothstep(outlineAlphaMax0, outlineAlphaMax1, d);
}
