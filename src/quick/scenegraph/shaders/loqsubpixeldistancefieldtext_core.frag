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

in vec3 sampleNearLeft;
in vec3 sampleNearRight;

out vec4 fragColor;

uniform sampler2D _qt_texture;
uniform vec4 color;
uniform float alphaMin;
uniform float alphaMax;

void main()
{
    vec2 n;
    n.x = textureProj(_qt_texture, sampleNearLeft).r;
    n.y = textureProj(_qt_texture, sampleNearRight).r;
    n = smoothstep(alphaMin, alphaMax, n);
    float c = 0.5 * (n.x + n.y);
    fragColor = vec4(n.x, c, n.y, c) * color.w;
}