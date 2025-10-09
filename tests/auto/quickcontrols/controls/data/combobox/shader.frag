// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(binding = 1) uniform sampler2D source; // this item

layout(std140, binding = 0) uniform buf {
    float qt_Opacity; // inherited opacity of this item
};


void main() {
    vec4 p = texture(source, qt_TexCoord0);
    lowp float g = dot(p.xyz, vec3(0.344, 0.5, 0.156));
    fragColor = vec4(g, g, g, p.a) * qt_Opacity;
}
