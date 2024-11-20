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

#version 120

uniform sampler2D _qt_texture;
uniform lowp float qt_Opacity;

#if defined(SPRITE)
varying highp vec4 fTexS;
#elif defined(DEFORM)
varying highp vec2 fTex;
#endif

#if defined(COLOR)
varying lowp vec4 fColor;
#else
varying lowp float fFade;
#endif

#if defined(TABLE)
varying lowp vec2 tt;
uniform sampler2D colortable;
#endif

void main()
{
#if defined(SPRITE)
    gl_FragColor = mix(texture2D(_qt_texture, fTexS.xy), texture2D(_qt_texture, fTexS.zw), tt.y)
            * fColor
            * texture2D(colortable, tt)
            * qt_Opacity;
#elif defined(TABLE)
    gl_FragColor = texture2D(_qt_texture, fTex)
            * fColor
            * texture2D(colortable, tt)
            * qt_Opacity;
#elif defined(DEFORM)
    gl_FragColor = (texture2D(_qt_texture, fTex)) * fColor * qt_Opacity;
#elif defined(COLOR)
    gl_FragColor = (texture2D(_qt_texture, gl_PointCoord)) * fColor * qt_Opacity;
#else
    gl_FragColor = texture2D(_qt_texture, gl_PointCoord) * (fFade * qt_Opacity);
#endif
}