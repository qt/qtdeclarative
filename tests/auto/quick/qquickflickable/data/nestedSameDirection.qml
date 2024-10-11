// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Flickable {
    id: root
    width: 240
    height: 320
    readonly property real flickableContentHeight: 500
    contentWidth: 800
    contentHeight: flickableContentHeight
    Column {
        Rectangle {
            width: 240
            height: root.flickableContentHeight / parent.children.length
            color: "lightsteelblue"

            Flickable {
                objectName: "nested1"
                anchors.fill: parent
                contentWidth: parent.width
                contentHeight: flickableContentHeight

                Repeater {
                    model: 3
                    Rectangle {
                        y: height * index
                        width: 96
                        height: root.flickableContentHeight / 3
                        color: Qt.rgba(Math.random() * 0.5, 0.5, 0.2, 1)
                        border.color: "black"
                    }
                }
            }
        }
        Rectangle {
            width: 240
            height: root.flickableContentHeight / parent.children.length
            color: "steelblue"

            Flickable {
                objectName: "nested2"
                anchors.fill: parent
                contentWidth: parent.width
                contentHeight: flickableContentHeight

                Repeater {
                    model: 3
                    Rectangle {
                        y: height * index
                        width: 96
                        height: root.flickableContentHeight / 3
                        color: Qt.rgba(Math.random() * 0.65, 0.4, 0.3, 1)
                        border.color: "black"
                    }
                }
            }
        }
    }
}
