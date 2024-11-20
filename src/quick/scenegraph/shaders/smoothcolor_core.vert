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

in vec4 vertex;
in vec4 vertexColor;
in vec2 vertexOffset;

out vec4 color;

uniform vec2 pixelSize;
uniform mat4 matrix;
uniform float opacity;

void main()
{
    vec4 pos = matrix * vertex;
    gl_Position = pos;

    if (vertexOffset.x != 0.) {
        vec4 delta = matrix[0] * vertexOffset.x;
        vec2 dir = delta.xy * pos.w - pos.xy * delta.w;
        vec2 ndir = .5 * pixelSize * normalize(dir / pixelSize);
        dir -= ndir * delta.w * pos.w;
        float numerator = dot(dir, ndir * pos.w * pos.w);
        float scale = 0.0;
        if (numerator < 0.0)
            scale = 1.0;
        else
            scale = min(1.0, numerator / dot(dir, dir));
        gl_Position += scale * delta;
    }

    if (vertexOffset.y != 0.) {
        vec4 delta = matrix[1] * vertexOffset.y;
        vec2 dir = delta.xy * pos.w - pos.xy * delta.w;
        vec2 ndir = .5 * pixelSize * normalize(dir / pixelSize);
        dir -= ndir * delta.w * pos.w;
        float numerator = dot(dir, ndir * pos.w * pos.w);
        float scale = 0.0;
        if (numerator < 0.0)
            scale = 1.0;
        else
            scale = min(1.0, numerator / dot(dir, dir));
        gl_Position += scale * delta;
    }

    color = vertexColor * opacity;
}