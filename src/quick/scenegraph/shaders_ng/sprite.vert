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

layout(location = 0) in vec2 vPos;
layout(location = 1) in vec2 vTex;

layout(location = 0) out vec4 fTexS;
layout(location = 1) out float progress;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    vec4 animPos; // x,y, x,y (two frames for interpolation)
    vec3 animData; // w,h(premultiplied of anim), interpolation progress
    float opacity;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    progress = ubuf.animData.z;

    // Calculate frame location in texture
    fTexS.xy = ubuf.animPos.xy + vTex.xy * ubuf.animData.xy;

    // Next frame is also passed, for interpolation
    fTexS.zw = ubuf.animPos.zw + vTex.xy * ubuf.animData.xy;

    gl_Position = ubuf.matrix * vec4(vPos.x, vPos.y, 0, 1);
}
