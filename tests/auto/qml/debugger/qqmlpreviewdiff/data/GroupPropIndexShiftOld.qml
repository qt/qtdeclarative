// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Group property (font) with no extra children.
// The font group sits at a certain index in the CU.
import QtQuick

Text {
    font.pixelSize: 12
    text: "hello"
    property int marker: 1
}
