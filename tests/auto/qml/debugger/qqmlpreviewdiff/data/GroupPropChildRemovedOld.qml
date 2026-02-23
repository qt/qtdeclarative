// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// A child exists before the font group property, then the child is removed.
// Reverse of GroupPropIndexShift: the Rectangle at index 1 disappears, so
// the content at index 1 changes from Rectangle to font-group in the new CU.
import QtQuick

Text {
    Rectangle { objectName: "removedChild"; width: 50 }
    font.pixelSize: 12
    text: "hello"
    property int marker: 1
}
