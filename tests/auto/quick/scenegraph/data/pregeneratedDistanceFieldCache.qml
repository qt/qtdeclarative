// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Rectangle {
    width: 320
    height: 200
    color: "white"

    property alias fontFamily: text.font.family

    Text {
        id: text
        anchors.centerIn: parent
        renderType: Text.QtRendering
        text: "ABC"
        color: "black"
        font.pixelSize: 64
    }
}
