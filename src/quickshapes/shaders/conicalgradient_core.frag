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

#define INVERSE_2PI 0.1591549430918953358

uniform sampler2D gradTabTexture;
uniform float opacity;

uniform float angle;

in vec2 coord;

out vec4 fragColor;

void main()
{
    float t;
    if (abs(coord.y) == abs(coord.x))
        t = (atan(-coord.y + 0.002, coord.x) + angle) * INVERSE_2PI;
    else
        t = (atan(-coord.y, coord.x) + angle) * INVERSE_2PI;
    fragColor = texture(gradTabTexture, vec2(t - floor(t), 0.5)) * opacity;
}
