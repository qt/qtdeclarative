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

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec2 multiTexCoord;
layout(location = 2) in vec2 vertexOffset;
layout(location = 3) in vec2 texCoordOffset;

layout(location = 0) out vec2 texCoord;
layout(location = 1) out float vertexOpacity;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float opacity;
    vec2 pixelSize;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    vec4 pos = ubuf.qt_Matrix * vertex;
    gl_Position = pos;
    texCoord = multiTexCoord;

    if (vertexOffset.x != 0.) {
        vec4 delta = ubuf.qt_Matrix[0] * vertexOffset.x;
        vec2 dir = delta.xy * pos.w - pos.xy * delta.w;
        vec2 ndir = .5 * ubuf.pixelSize * normalize(dir / ubuf.pixelSize);
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
        vec4 delta = ubuf.qt_Matrix[1] * vertexOffset.y;
        vec2 dir = delta.xy * pos.w - pos.xy * delta.w;
        vec2 ndir = .5 * ubuf.pixelSize * normalize(dir / ubuf.pixelSize);
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
        vertexOpacity = ubuf.opacity;
}
