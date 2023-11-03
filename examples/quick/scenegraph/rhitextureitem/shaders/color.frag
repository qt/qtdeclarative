// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#version 440

layout(location = 0) in vec3 v_color;

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = vec4(v_color, 1.0);
}
