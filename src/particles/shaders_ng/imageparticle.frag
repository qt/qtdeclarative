// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#version 440

#if defined(TABLE)
layout(location = 0) in vec2 tt;
#endif

#if defined(SPRITE)
layout(location = 1) in vec4 fTexS;
#elif !defined(POINT)
layout(location = 1) in vec2 fTex;
#endif

#if defined(COLOR)
layout(location = 2) in vec4 fColor;
#else
layout(location = 2) in float fFade;
#endif

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
#if QSHADER_VIEW_COUNT >= 2
    mat4 matrix[QSHADER_VIEW_COUNT];
#else
    mat4 matrix;
#endif
    float opacity;
    float entry;
    float timestamp;
    float dpr;
    float sizetable[64];
    float opacitytable[64];
} ubuf;

layout(binding = 1) uniform sampler2D _qt_texture;

#if defined(TABLE) || defined(SPRITE)
layout(binding = 2) uniform sampler2D colortable;
#endif

void main()
{
#if defined(SPRITE)
    fragColor = mix(texture(_qt_texture, fTexS.xy), texture(_qt_texture, fTexS.zw), tt.y)
              * fColor
              * texture(colortable, tt)
              * ubuf.opacity;
#elif defined(TABLE)
    fragColor = texture(_qt_texture, fTex)
              * fColor
              * texture(colortable, tt)
              * ubuf.opacity;
#elif !defined(POINT)
    fragColor = texture(_qt_texture, fTex) * fColor * ubuf.opacity;
#elif defined(COLOR)
    fragColor = texture(_qt_texture, gl_PointCoord) * fColor * ubuf.opacity;
#else // simple point
    fragColor = texture(_qt_texture, gl_PointCoord) * fFade * ubuf.opacity;
#endif
}
