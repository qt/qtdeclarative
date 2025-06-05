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

#version 150 core

in vec2 sampleCoord;
in vec3 sampleFarLeft;
in vec3 sampleNearLeft;
in vec3 sampleNearRight;
in vec3 sampleFarRight;

out vec4 fragColor;

uniform sampler2D _qt_texture;
uniform vec4 color;
uniform float alphaMin;
uniform float alphaMax;

void main()
{
    vec4 n;
    n.x = textureProj(_qt_texture, sampleFarLeft).r;
    n.y = textureProj(_qt_texture, sampleNearLeft).r;
    float c = texture(_qt_texture, sampleCoord).r;
    n.z = textureProj(_qt_texture, sampleNearRight).r;
    n.w = textureProj(_qt_texture, sampleFarRight).r;

    vec2 d = min(abs(n.yw - n.xz) * 2., 0.67);
    vec2 lo = mix(vec2(alphaMin), vec2(0.5), d);
    vec2 hi = mix(vec2(alphaMax), vec2(0.5), d);
    n = smoothstep(lo.xxyy, hi.xxyy, n);
    c = smoothstep(lo.x + lo.y, hi.x + hi.y, 2. * c);

    fragColor = vec4(0.333 * (n.xyz + n.yzw + c), c) * color.w;
}