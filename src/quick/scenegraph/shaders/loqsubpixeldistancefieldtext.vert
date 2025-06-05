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

uniform highp mat4 matrix;
uniform highp vec2 textureScale;
uniform highp float fontScale;
uniform highp vec4 vecDelta;

attribute highp vec4 vCoord;
attribute highp vec2 tCoord;

varying highp vec3 sampleNearLeft;
varying highp vec3 sampleNearRight;

void main()
{
    highp vec2 sampleCoord = tCoord * textureScale;
    gl_Position = matrix * vCoord;

    // Calculate neighbor pixel position in item space.
    highp vec3 wDelta = gl_Position.w * vecDelta.xyw;
    highp vec3 nearLeft = vCoord.xyw - 0.25 * wDelta;
    highp vec3 nearRight = vCoord.xyw + 0.25 * wDelta;

    // Calculate neighbor texture coordinate.
    highp vec2 scale = textureScale / fontScale;
    highp vec2 base = sampleCoord - scale * vCoord.xy;
    sampleNearLeft = vec3(base * nearLeft.z + scale * nearLeft.xy, nearLeft.z);
    sampleNearRight = vec3(base * nearRight.z + scale * nearRight.xy, nearRight.z);
}