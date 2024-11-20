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

varying highp vec3 sampleNearLeft;
varying highp vec3 sampleNearRight;

uniform sampler2D _qt_texture;
uniform lowp vec4 color;
uniform mediump float alphaMin;
uniform mediump float alphaMax;

void main()
{
    highp vec2 n;
    n.x = texture2DProj(_qt_texture, sampleNearLeft).a;
    n.y = texture2DProj(_qt_texture, sampleNearRight).a;
    n = smoothstep(alphaMin, alphaMax, n);
    highp float c = 0.5 * (n.x + n.y);
    gl_FragColor = vec4(n.x, c, n.y, c) * color.w;
}