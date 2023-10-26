// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [1]
#version 440

layout(location = 0) in vec2 vTexCoord;

layout(location = 0) out vec4 fragColor;

//! [2]
// uniform block: 84 bytes
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix; // offset 0
    float qt_Opacity; // offset 64
    float zoom; // offset 68
    vec2 center; // offset 72
    int limit; // offset 80
} ubuf;
//! [2]

void main()
{
    vec4 color1 = vec4(1.0, 0.85, 0.55, 1);
    vec4 color2 = vec4(0.226, 0.0, 0.615, 1);

    float aspect_ratio = -ubuf.qt_Matrix[0][0]/ubuf.qt_Matrix[1][1];
    vec2 z, c;

    c.x = (vTexCoord.x - 0.5) / ubuf.zoom + ubuf.center.x;
    c.y = aspect_ratio * (vTexCoord.y - 0.5) / ubuf.zoom + ubuf.center.y;

    int i;
    z = c;
    for (i = 0; i < ubuf.limit; i++) {
        float x = (z.x * z.x - z.y * z.y) + c.x;
        float y = (z.y * z.x + z.x * z.y) + c.y;

        if ((x * x + y * y) > 4.0) break;
        z.x = x;
        z.y = y;
    }

    if (i == ubuf.limit) {
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        float f = (i * 1.0) / ubuf.limit;
        fragColor = mix(color1, color2, sqrt(f));
    }
}
//! [1]
