// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    width: 400
    height: 240
    color: "white"

    ListModel {
        id: appModel
        ListElement {
            name: "Music"
            icon: "pics/AudioPlayer_48.png"
        }
        ListElement {
            name: "Movies"
            icon: "pics/VideoPlayer_48.png"
        }
        ListElement {
            name: "Camera"
            icon: "pics/Camera_48.png"
        }
        ListElement {
            name: "Calendar"
            icon: "pics/DateBook_48.png"
        }
        ListElement {
            name: "Messaging"
            icon: "pics/EMail_48.png"
        }
        ListElement {
            name: "Todo List"
            icon: "pics/TodoList_48.png"
        }
        ListElement {
            name: "Contacts"
            icon: "pics/AddressBook_48.png"
        }
    }

    Component {
        id: appDelegate
        Item {
            width: 100
            height: 100
            scale: PathView.iconScale

            required property string name
            required property string icon
            required property int index

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
                onClicked: view.currentIndex = parent.index
            }
        }
    }

    Component {
        id: appHighlight
        Rectangle {
            width: 80
            height: 80
            color: "lightsteelblue"
        }
    }

    PathView {
        id: view
        anchors.fill: parent
        highlight: appHighlight
        preferredHighlightBegin: 0.5
        preferredHighlightEnd: 0.5
        focus: true
        model: appModel
        delegate: appDelegate
        path: Path {
            startX: 10
            startY: 50
            PathAttribute {
                name: "iconScale"
                value: 0.5
            }
            PathQuad {
                x: 200
                y: 150
                controlX: 50
                controlY: 200
            }
            PathAttribute {
                name: "iconScale"
                value: 1.0
            }
            PathQuad {
                x: 390
                y: 50
                controlX: 350
                controlY: 200
            }
            PathAttribute {
                name: "iconScale"
                value: 0.5
            }
        }
    }
}
