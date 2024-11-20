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

layout(location = 0) in vec4 vertexCoord;
layout(location = 1) in vec4 vertexColor;

layout(location = 0) out float gradTabIndex;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    vec2 gradStart;
    vec2 gradEnd;
    float opacity;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    vec2 gradVec = ubuf.gradEnd - ubuf.gradStart;
    gradTabIndex = dot(gradVec, vertexCoord.xy - ubuf.gradStart) / (gradVec.x * gradVec.x + gradVec.y * gradVec.y);
    gl_Position = ubuf.matrix * vertexCoord;
}
