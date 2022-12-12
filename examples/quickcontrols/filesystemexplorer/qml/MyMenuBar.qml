// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import FileSystemModule

// The MenuBar also serves as a controller for our Window as we don't use any decorations.
MenuBar {
    id: root

    required property ApplicationWindow rootWindow
    property alias infoText: windowInfo.text

    implicitHeight: 25

    // The top level menus on the left side
    delegate: MenuBarItem {
        id: menuBarItem
        implicitHeight: 25

        contentItem: Text {
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            color: menuBarItem.highlighted ? Colors.textFile : Colors.text
            opacity: enabled ? 1.0 : 0.3
            text: menuBarItem.text
            elide: Text.ElideRight
            font: menuBarItem.font
        }

        background: Rectangle {
            color: menuBarItem.highlighted ? Colors.selection : "transparent"
            Rectangle {
                id: indicator
                width: 0; height: 3
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                color: Colors.color1

                states: State {
                    name: "active"; when: menuBarItem.highlighted
                    PropertyChanges { target: indicator; width: parent.width }
                }

                transitions: Transition {
                    NumberAnimation {
                        properties: "width"
                        duration: 300
                    }
                }

            }
        }
    }

    // The background property contains an information text in the middle as well as the
    // Minimize, Maximize and Close Buttons.
    background: Rectangle {
        color: Colors.surface2
        // Make the empty space drag the specified root window.
        WindowDragHandler { dragWindow: rootWindow }

        Text {
            id: windowInfo
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            color: Colors.text
        }

        component InteractionButton: Rectangle {
            signal action;
            property alias hovered: hoverHandler.hovered

            width: root.height
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            color: hovered ? Colors.background : "transparent"

            HoverHandler { id: hoverHandler }
            TapHandler { onTapped: action() }
        }

        InteractionButton {
            id: minimize

            anchors.right: maximize.left
            onAction: rootWindow.showMinimized()
            Rectangle {
                width: parent.height - 10; height: 2
                anchors.centerIn: parent
                color: parent.hovered ? Colors.iconIndicator : Colors.icon
            }
        }

        InteractionButton {
            id: maximize

            anchors.right: close.left
            onAction: rootWindow.showMaximized()
            Rectangle {
                anchors.fill: parent
                anchors.margins: 5
                border.width: 2
                color: "transparent"
                border.color: parent.hovered ? Colors.iconIndicator : Colors.icon
            }
        }

        InteractionButton {
            id: close

            color: hovered ? "#ec4143" : "transparent"
            anchors.right: parent.right
            onAction: rootWindow.close()
            Rectangle {
                width: parent.height - 8; height: 2
                anchors.centerIn: parent
                color: parent.hovered ? Colors.iconIndicator : Colors.icon
                rotation: 45
                transformOrigin: Item.Center
                antialiasing: true
                Rectangle {
                    width: parent.height
                    height: parent.width
                    anchors.centerIn: parent
                    color: parent.color
                    antialiasing: true
                }
            }
        }
    }

}
