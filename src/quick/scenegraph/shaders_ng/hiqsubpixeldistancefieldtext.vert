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

layout(location = 0) in vec4 vCoord;
layout(location = 1) in vec2 tCoord;

layout(location = 0) out vec2 sampleCoord;
layout(location = 1) out vec3 sampleFarLeft;
layout(location = 2) out vec3 sampleNearLeft;
layout(location = 3) out vec3 sampleNearRight;
layout(location = 4) out vec3 sampleFarRight;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    vec2 textureScale;
    vec4 color;
    float alphaMin;
    float alphaMax;
    // up to this point it must match distancefieldtext
    float fontScale;
    vec4 vecDelta;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    sampleCoord = tCoord * ubuf.textureScale;
    gl_Position = ubuf.matrix * vCoord;

    // Calculate neighbor pixel position in item space.
    vec3 wDelta = gl_Position.w * ubuf.vecDelta.xyw;
    vec3 farLeft = vCoord.xyw - 0.667 * wDelta;
    vec3 nearLeft = vCoord.xyw - 0.333 * wDelta;
    vec3 nearRight = vCoord.xyw + 0.333 * wDelta;
    vec3 farRight = vCoord.xyw + 0.667 * wDelta;

    // Calculate neighbor texture coordinate.
    vec2 scale = ubuf.textureScale / ubuf.fontScale;
    vec2 base = sampleCoord - scale * vCoord.xy;
    sampleFarLeft = vec3(base * farLeft.z + scale * farLeft.xy, farLeft.z);
    sampleNearLeft = vec3(base * nearLeft.z + scale * nearLeft.xy, nearLeft.z);
    sampleNearRight = vec3(base * nearRight.z + scale * nearRight.xy, nearRight.z);
    sampleFarRight = vec3(base * farRight.z + scale * farRight.xy, farRight.z);
}
