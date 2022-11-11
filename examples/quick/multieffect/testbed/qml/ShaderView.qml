// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    id: rootItem

    property string text

    width: textItem.width
    height: textItem.height

    Rectangle {
        anchors.fill: textItem
        anchors.margins: -10
        z: -1
        color: "#000000"
        opacity: 0.6
        border.color: "#ffffff"
        border.width: 2
    }
    Text {
        id: textItem
        text: rootItem.text
        font.pixelSize: 16
        color: "#ffffff"
    }
}
