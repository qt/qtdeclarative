// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#version 440

layout(location = 0) in vec2 texCoord;
layout(location = 1) in float vertexOpacity;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D qt_Texture;

void main()
{
    fragColor = texture(qt_Texture, texCoord) * vertexOpacity;
}
