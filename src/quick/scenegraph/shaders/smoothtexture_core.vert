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
in vec2 multiTexCoord;
in vec2 vertexOffset;
in vec2 texCoordOffset;

out vec2 texCoord;
out float vertexOpacity;

uniform vec2 pixelSize;
uniform mat4 qt_Matrix;
uniform float opacity;

void main()
{
    vec4 pos = qt_Matrix * vertex;
    gl_Position = pos;
    texCoord = multiTexCoord;

    if (vertexOffset.x != 0.) {
        vec4 delta = qt_Matrix[0] * vertexOffset.x;
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
        texCoord.x += scale * texCoordOffset.x;
    }

    if (vertexOffset.y != 0.) {
        vec4 delta = qt_Matrix[1] * vertexOffset.y;
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
        texCoord.y += scale * texCoordOffset.y;
    }

    bool onEdge = any(notEqual(vertexOffset, vec2(0.)));
    bool outerEdge = all(equal(texCoordOffset, vec2(0.)));
    if (onEdge && outerEdge)
        vertexOpacity = 0.;
    else
        vertexOpacity = opacity;
}
