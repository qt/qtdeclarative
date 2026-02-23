// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Multiple group properties (font and anchors) on the same object.
import QtQuick

Text {
    font.pixelSize: 12
    font.bold: false
    anchors.leftMargin: 5
    text: "hello"
}
