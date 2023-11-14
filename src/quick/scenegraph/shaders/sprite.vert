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

attribute highp vec2 vPos;
attribute highp vec2 vTex;

uniform highp vec3 animData;// w,h(premultiplied of anim), interpolation progress
uniform highp vec4 animPos;//x,y, x,y (two frames for interpolation)

uniform highp mat4 qt_Matrix;

varying highp vec4 fTexS;
varying lowp float progress;

void main()
{
    progress = animData.z;

    // Calculate frame location in texture
    fTexS.xy = animPos.xy + vTex.xy * animData.xy;

    // Next frame is also passed, for interpolation
    fTexS.zw = animPos.zw + vTex.xy * animData.xy;

    gl_Position = qt_Matrix * vec4(vPos.x, vPos.y, 0, 1);
}