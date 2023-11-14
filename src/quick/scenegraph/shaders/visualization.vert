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

attribute highp vec4 v;
uniform highp mat4 matrix;
uniform highp mat4 rotation;

// set to 1 if projection is enabled
uniform bool projection;

varying mediump vec2 pos;

void main()
{
    vec4 p = matrix * v;

    if (projection) {
        vec4 proj = rotation * p;
        gl_Position = vec4(proj.x, proj.y, 0, proj.z);
    } else {
        gl_Position = p;
    }

    pos = v.xy * 1.37;
}
