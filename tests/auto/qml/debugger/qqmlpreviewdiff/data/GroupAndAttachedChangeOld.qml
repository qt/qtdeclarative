// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Both a group property (font) and an attached property (Keys) on the same object.
import QtQuick

Text {
    font.pixelSize: 12
    Keys.enabled: true
    text: "hello"
    property int marker: 1
}
