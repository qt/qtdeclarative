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

#if defined(SPRITE)
in vec4 fTexS;
#elif defined(DEFORM)
in vec2 fTex;
#endif

#if defined(COLOR)
in vec4 fColor;
#else
in float fFade;
#endif

#if defined(TABLE)
in vec2 tt;
uniform sampler2D colortable;
#endif

out vec4 fragColor;

uniform sampler2D _qt_texture;
uniform float qt_Opacity;

void main()
{
#if defined(SPRITE)
    fragColor = mix(texture(_qt_texture, fTexS.xy), texture(_qt_texture, fTexS.zw), tt.y)
              * fColor
              * texture(colortable, tt)
              * qt_Opacity;
#elif defined(TABLE)
    fragColor = texture(_qt_texture, fTex)
              * fColor
              * texture(colortable, tt)
              * qt_Opacity;
#elif defined(DEFORM)
    fragColor = texture(_qt_texture, fTex) * fColor * qt_Opacity;
#elif defined(COLOR)
    fragColor = texture(_qt_texture, gl_PointCoord) * fColor * qt_Opacity;
#else
    fragColor = texture(_qt_texture, gl_PointCoord) * fFade * qt_Opacity;
#endif
}