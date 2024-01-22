// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#version 440

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec3 barycentric;

layout(std140, binding = 0) uniform buf {
#if QSHADER_VIEW_COUNT >= 2
    mat4 qt_Matrix[QSHADER_VIEW_COUNT];
#else
    mat4 qt_Matrix;
#endif
} ubuf;

void main()
{
    float f = min(barycentric.x, min(barycentric.y, barycentric.z));
    float d = fwidth(f * 1.5);
    float alpha = smoothstep(0.0, d, f);

    //alpha = 1.0 - step(0.5, barycentric.x);
    fragColor = vec4(1.0, 0.2, 1.0, 1.0) * (1.0 - alpha);
}
