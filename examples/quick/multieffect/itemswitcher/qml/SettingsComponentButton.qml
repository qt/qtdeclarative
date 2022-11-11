// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    id: rootItem

    property alias text: textItem.text
    property bool selected: false

    signal clicked

    width: parent.width
    height: 40 * dp
    Rectangle {
        anchors.fill: parent
        color: "#606060"
        border.color: "#d0d0d0"
        border.width: 1
        opacity: selected ? 0.8 : 0.4
    }
    Text {
        id: textItem
        anchors.centerIn: parent
        font.pixelSize: 16 * dp
        color: "#f0f0f0"
    }
    MouseArea {
        anchors.fill: parent
        onClicked: {
            rootItem.clicked();
        }
    }
}
