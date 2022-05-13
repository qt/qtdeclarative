// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: button
    property alias text: textItem.text
    signal clicked()

    readonly property color constantColor: "#63ACBE"
    color: mouseArea.pressed ? Qt.lighter(constantColor) : constantColor
    width: textItem.implicitWidth + 5
    height: textItem.implicitHeight + 5
    radius: 10

    Text {
        id: textItem
        font.pixelSize: 22
        color: "black"
        anchors.centerIn: button
    }

    MouseArea {
        id: mouseArea
        anchors.fill: button
        anchors.margins: -5
        onClicked: function(event) { button.clicked(); }
    }
}
