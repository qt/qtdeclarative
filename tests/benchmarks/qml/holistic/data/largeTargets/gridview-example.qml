// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.0

Rectangle {
    width: 300; height: 400
    color: "white"

    ListModel {
        id: appModel
        ListElement { name: "Music"; shade: "blue" }
        ListElement { name: "Movies"; shade: "red" }
        ListElement { name: "Camera"; shade: "green" }
        ListElement { name: "Calendar"; shade: "yellow" }
        ListElement { name: "Messaging"; shade: "cyan" }
        ListElement { name: "Todo List"; shade: "magenta" }
        ListElement { name: "Contacts"; shade: "black" }
    }

    Component {
        id: appDelegate

        Item {
            width: 100; height: 100

            Rectangle {
                id: myColoredIcon
                width: 20
                height: 20
                y: 20; anchors.horizontalCenter: parent.horizontalCenter
                color: shade
            }
            Text {
                anchors { top: myColoredIcon.bottom; horizontalCenter: parent.horizontalCenter }
                text: name
            }
        }
    }

    Component {
        id: appHighlight
        Rectangle { width: 80; height: 80; color: "lightsteelblue" }
    }

    GridView {
        anchors.fill: parent
        cellWidth: 100; cellHeight: 100
        highlight: appHighlight
        focus: true
        model: appModel
        delegate: appDelegate
    }
}
