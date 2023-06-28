// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    width: 300
    height: 400
    color: "white"

    ListModel {
        id: appModel
        ListElement { name: "Music"; icon: "pics/AudioPlayer_48.png" }
        ListElement { name: "Movies"; icon: "pics/VideoPlayer_48.png" }
        ListElement { name: "Camera"; icon: "pics/Camera_48.png" }
        ListElement { name: "Calendar"; icon: "pics/DateBook_48.png" }
        ListElement { name: "Messaging"; icon: "pics/EMail_48.png" }
        ListElement { name: "Todo List"; icon: "pics/TodoList_48.png" }
        ListElement { name: "Contacts"; icon: "pics/AddressBook_48.png" }
    }
//! [0]
    GridView {
        anchors.fill: parent
        cellWidth: 100
        cellHeight: 100
        focus: true
        model: appModel

        highlight: Rectangle {
            width: 80
            height: 80
            color: "lightsteelblue"
        }

        delegate: Item {
            required property string icon
            required property string name
            required property int index

            width: 100
            height: 100

            Image {
                id: myIcon
                y: 20
                anchors.horizontalCenter: parent.horizontalCenter
                source: parent.icon
            }
            Text {
                anchors {
                    top: myIcon.bottom
                    horizontalCenter: parent.horizontalCenter
                }
                text: parent.name
            }
            MouseArea {
                anchors.fill: parent
                onClicked: parent.GridView.view.currentIndex = parent.index
            }
        }
    }
//! [0]
}
