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

attribute highp vec2 qt_ParticlePos;
attribute highp vec2 qt_ParticleTex;
attribute highp vec4 qt_ParticleData; // x = time, y = lifeSpan, z = size, w = endSize
attribute highp vec4 qt_ParticleVec;  // x,y = constant velocity, z,w = acceleration
attribute highp float qt_ParticleR;

uniform highp mat4 qt_Matrix;
uniform highp float qt_Timestamp;

varying highp vec2 qt_TexCoord0;

void defaultMain()
{
    qt_TexCoord0 = qt_ParticleTex;
    highp float size = qt_ParticleData.z;
    highp float endSize = qt_ParticleData.w;
    highp float t = (qt_Timestamp - qt_ParticleData.x) / qt_ParticleData.y;
    highp float currentSize = mix(size, endSize, t * t);
    if (t < 0. || t > 1.)
        currentSize = 0.;
    highp vec2 pos = qt_ParticlePos
                   - currentSize / 2. + currentSize * qt_ParticleTex   // adjust size
                   + qt_ParticleVec.xy * t * qt_ParticleData.y         // apply velocity vector..
                   + 0.5 * qt_ParticleVec.zw * pow(t * qt_ParticleData.y, 2.);
    gl_Position = qt_Matrix * vec4(pos.x, pos.y, 0, 1);
}
