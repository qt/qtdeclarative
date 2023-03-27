// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

varying highp vec2 qt_TexCoord0;
uniform highp float qt_Opacity;
uniform lowp sampler2D source;
uniform lowp sampler2D maskSource;
void main(void) {
    gl_FragColor = texture2D(source, qt_TexCoord0.st) * (texture2D(maskSource, qt_TexCoord0.st).a) * qt_Opacity;
}
