// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    id: container

    required property string text
    property string subText: ""
    signal clicked

    width: buttonLabel.width + 20; height: col.height + 12

    MouseArea {
        id: mouseArea;
        anchors.fill: parent;
        onClicked: container.clicked();
        onPressed: background.color = Qt.darker("lightgrey");
        onReleased: background.color="lightgrey";
    }

    Rectangle {
        id: background
        anchors.fill: parent
        color: "lightgrey"
        radius: 4
        border.width: 1
        border.color: Qt.darker(color)
    }

    Column {
        spacing: 2
        id: col
        x: 10
        y: 6
        Text {
            id: buttonLabel; text: container.text; color: "black"; font.pixelSize: 24
        }
        Text {
            id: buttonLabel2; text: container.subText; color: "black"; font.pixelSize: 12
        }
    }
}
