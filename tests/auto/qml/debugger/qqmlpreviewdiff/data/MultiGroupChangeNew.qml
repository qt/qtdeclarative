// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Multiple group properties (font and anchors) on the same object, both changed.
import QtQuick

Text {
    font.pixelSize: 24
    font.bold: true
    anchors.leftMargin: 10
    text: "hello"
}
