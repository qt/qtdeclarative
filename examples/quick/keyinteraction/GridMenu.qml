// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma ComponentBehavior: Bound
import QtQuick

FocusScope {
    id: menu

    property alias interactive: gridView.interactive
    required property Item keyUpTarget
    required property Item keyDownTarget
    required property Item keyLeftTarget

    Rectangle {
        anchors.fill: parent
        clip: true
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: "#193441"
            }
            GradientStop {
                position: 1.0
                color: Qt.darker("#193441")
            }
        }

        GridView {
            id: gridView

            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            cellWidth: 152
            cellHeight: 152
            focus: true
            model: 12

            KeyNavigation.up: menu.keyUpTarget
            KeyNavigation.down: menu.keyDownTarget
            KeyNavigation.left: menu.keyLeftTarget

            delegate: Item {
                id: container

                width: gridView.cellWidth
                height: gridView.cellHeight
                required property int index

                Rectangle {
                    id: content

                    color: "transparent"
                    antialiasing: true
                    anchors.fill: parent
                    anchors.margins: 20
                    radius: 10

                    Rectangle {
                        color: "#91AA9D"
                        anchors.fill: parent
                        anchors.margins: 3
                        radius: 8
                        antialiasing: true
                    }
                    Image {
                        source: "images/qt-logo.png"
                        anchors.centerIn: parent
                    }
                }

                MouseArea {
                    id: mouseArea

                    anchors.fill: parent
                    hoverEnabled: true

                    onClicked: {
                        container.GridView.view.currentIndex = container.index
                        container.forceActiveFocus()
                    }
                }

                states: State {
                    name: "active"
                    when: container.activeFocus
                    PropertyChanges {
                        content {
                            color: "#FCFFF5"
                            scale: 1.1
                        }
                    }
                }

                transitions: Transition {
                    NumberAnimation {
                        properties: "scale"
                        duration: 100
                    }
                }
            }
        }
    }
}
