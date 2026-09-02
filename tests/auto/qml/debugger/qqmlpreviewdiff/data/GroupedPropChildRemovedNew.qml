// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// The child is removed; the content at index 1 changes from Rectangle to font-group.
import QtQuick

Text {
    font.pixelSize: 24
    text: "hello"
    property int marker: 2
}
