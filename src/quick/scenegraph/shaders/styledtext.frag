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

varying highp vec2 sampleCoord;
varying highp vec2 shiftedSampleCoord;

uniform sampler2D _qt_texture;
uniform lowp vec4 color;
uniform lowp vec4 styleColor;

void main()
{
    lowp float glyph = texture2D(_qt_texture, sampleCoord).a;
    lowp float style = clamp(texture2D(_qt_texture, shiftedSampleCoord).a - glyph,
                             0.0, 1.0);
    gl_FragColor = style * styleColor + glyph * color;
}