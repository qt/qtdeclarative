/****************************************************************************
**
** Copyright (C) 2023 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
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