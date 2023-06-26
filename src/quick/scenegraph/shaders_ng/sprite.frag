// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#version 440

layout(location = 0) in vec4 fTexS;
layout(location = 1) in float progress;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D tex;

layout(std140, binding = 0) uniform buf {
#if QSHADER_VIEW_COUNT >= 2
    mat4 matrix[QSHADER_VIEW_COUNT];
#else
    mat4 matrix;
#endif
    vec4 animPos;
    vec3 animData;
    float opacity;
};

void main()
{
    fragColor = mix(texture(tex, fTexS.xy),
                    texture(tex, fTexS.zw),
                    progress) * opacity;
}
