// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#version 450

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float hue;
};

float hueToIntensity(float v1, float v2, float h) {
    h = fract(h);
    if (h < 1.0 / 6.0)
        return v1 + (v2 - v1) * 6.0 * h;
    else if (h < 1.0 / 2.0)
        return v2;
    else if (h < 2.0 / 3.0)
        return v1 + (v2 - v1) * 6.0 * (2.0 / 3.0 - h);

    return v1;
}

vec3 HSLtoRGB(vec3 color) {
    float h = color.x;
    float l = color.z;
    float s = color.y;

    if (s < 1.0 / 256.0)
        return vec3(l, l, l);

    float v1;
    float v2;
    if (l < 0.5)
        v2 = l * (1.0 + s);
    else
        v2 = (l + s) - (s * l);

    v1 = 2.0 * l - v2;

    float d = 1.0 / 3.0;
    float r = hueToIntensity(v1, v2, h + d);
    float g = hueToIntensity(v1, v2, h);
    float b = hueToIntensity(v1, v2, h - d);
    return vec3(r, g, b);
}

void main() {
    vec4 c = vec4(1.0);
    c.rgb = HSLtoRGB(vec3(hue, 1.0 - qt_TexCoord0.t, qt_TexCoord0.s));
    fragColor = c * qt_Opacity;
}
