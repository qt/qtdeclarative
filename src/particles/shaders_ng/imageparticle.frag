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

#version 440

#if defined(TABLE)
layout(location = 0) in vec2 tt;
#endif

#if defined(SPRITE)
layout(location = 1) in vec4 fTexS;
#elif defined(DEFORM)
layout(location = 1) in vec2 fTex;
#endif

#if defined(COLOR)
layout(location = 2) in vec4 fColor;
#else
layout(location = 2) in float fFade;
#endif

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    float opacity;
    float entry;
    float timestamp;
    float sizetable[64];
    float opacitytable[64];
} ubuf;

layout(binding = 1) uniform sampler2D _qt_texture;

#if defined(TABLE) || defined(SPRITE)
layout(binding = 2) uniform sampler2D colortable;
#endif

void main()
{
#if defined(SPRITE)
    fragColor = mix(texture(_qt_texture, fTexS.xy), texture(_qt_texture, fTexS.zw), tt.y)
              * fColor
              * texture(colortable, tt)
              * ubuf.opacity;
#elif defined(TABLE)
    fragColor = texture(_qt_texture, fTex)
              * fColor
              * texture(colortable, tt)
              * ubuf.opacity;
#elif defined(DEFORM)
    fragColor = texture(_qt_texture, fTex) * fColor * ubuf.opacity;
#elif defined(COLOR)
    fragColor = texture(_qt_texture, gl_PointCoord) * fColor * ubuf.opacity;
#else
    fragColor = texture(_qt_texture, gl_PointCoord) * fFade * ubuf.opacity;
#endif
}
