// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.0

PathView {
    id: view
    width: 400; height: 240
    highlight: Rectangle { width: 80; height: 80; color: "lightsteelblue" }
    preferredHighlightBegin: 0.5
    preferredHighlightEnd: 0.5
    model: ListModel {
        id: appModel
        ListElement { name: "Music" }
        ListElement { name: "Movies" }
        ListElement { name: "Camera" }
        ListElement { name: "Calendar" }
        ListElement { name: "Messaging" }
        ListElement { name: "Todo List" }
        ListElement { name: "Contacts" }
    }
    delegate: Rectangle {
        width: 100; height: 100
        scale: PathView.iconScale
        border.color: "lightgrey"
        color: "transparent"
        Text {
            anchors { horizontalCenter: parent.horizontalCenter }
            text: name
            smooth: true
            color: ma.pressed ? "red" : "black"
        }

        MouseArea {
            id: ma
            anchors.fill: parent
            onClicked: view.currentIndex = index
        }
    }
    path: Path {
        startX: 10
        startY: 50
        PathAttribute { name: "iconScale"; value: 0.5 }
        PathQuad { x: 200; y: 150; controlX: 50; controlY: 200 }
        PathAttribute { name: "iconScale"; value: 1.0 }
        PathQuad { x: 390; y: 50; controlX: 350; controlY: 200 }
        PathAttribute { name: "iconScale"; value: 0.5 }
    }
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 20
        text: view.currentIndex + " @ " + offset.toFixed(2)
    }
}
