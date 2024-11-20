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

layout(location = 0) in vec4 v;
layout(location = 0) out vec2 pos;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    mat4 rotation;
    vec4 color;
    float pattern;
    int projection;
} ubuf;

out gl_PerVertex { vec4 gl_Position; float gl_PointSize; };

void main()
{
    vec4 p = ubuf.matrix * v;

    if (ubuf.projection != 0) {
        vec4 proj = ubuf.rotation * p;
        gl_Position = vec4(proj.x, proj.y, 0, proj.z);
    } else {
        gl_Position = p;
    }

    pos = v.xy * 1.37;

    gl_PointSize = 1.0;
}
