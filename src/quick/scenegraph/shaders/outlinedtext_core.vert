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

in vec4 vCoord;
in vec2 tCoord;

out vec2 sampleCoord;
out vec2 sCoordUp;
out vec2 sCoordDown;
out vec2 sCoordLeft;
out vec2 sCoordRight;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform vec2 textureScale;
uniform vec2 shift;
uniform float dpr;

void main()
{
     sampleCoord = tCoord * textureScale;
     sCoordUp = (tCoord - vec2(0.0, -1.0)) * textureScale;
     sCoordDown = (tCoord - vec2(0.0, 1.0)) * textureScale;
     sCoordLeft = (tCoord - vec2(-1.0, 0.0)) * textureScale;
     sCoordRight = (tCoord - vec2(1.0, 0.0)) * textureScale;
     vec4 xformed = modelViewMatrix * vCoord;
     gl_Position = projectionMatrix * vec4(floor(xformed.xyz * dpr + 0.5) / dpr, xformed.w);
}
