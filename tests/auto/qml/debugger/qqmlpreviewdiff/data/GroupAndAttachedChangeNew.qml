// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Both a group property (font) and an attached property (Keys) changed.
import QtQuick

Text {
    font.pixelSize: 24
    Keys.enabled: false
    text: "hello"
    property int marker: 2
}
