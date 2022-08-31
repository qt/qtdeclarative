// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    id: selector

    property alias list: view.model
    property alias selectedIndex: view.currentIndex
    property alias label: labelText.text
    property bool expanded

    width: 100; height: labelText.implicitHeight + 26

    Rectangle {
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom; }

        height: labelText.implicitHeight + 4 + (expanded ? 20 * view.count : 20)
        Behavior on height { NumberAnimation { duration: 300 } }

        radius: 2
        border.width: 1
        border.color: "yellow"
        color: "yellow"

        MouseArea {
            anchors.fill: parent

            onClicked: selector.expanded = !selector.expanded

            Text {
                id: labelText
                anchors { left: parent.left; top: parent.top; margins: 2 }
            }

            Rectangle {
                anchors {
                    left: parent.left; top: labelText.bottom;
                    right: parent.right; bottom: parent.bottom;
                    margins: 2
                    leftMargin: 10
                }

                radius: 2
                color: "white"

                ListView {
                    id: view

                    anchors.fill: parent

                    clip: true

                    delegate: Text {
                        anchors { left: parent.left; right: parent.right }
                        height: 20

                        verticalAlignment: Text.AlignVCenter

                        text: modelData

                        MouseArea {
                            anchors.fill: parent

                            onClicked: {
                                view.currentIndex = index
                                selector.expanded = !selector.expanded
                            }
                        }
                    }
                    highlight: Rectangle {
                        anchors { left: parent.left; right: parent.right }
                        height: 20
                        radius: 2

                        color: "yellow"
                    }
                }
            }
        }
    }
}
