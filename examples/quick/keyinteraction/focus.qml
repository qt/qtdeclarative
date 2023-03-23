// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQml
import QtQuick

Rectangle {
    id: window

    width: 800
    height: 640
    color: "#3E606F"

    FocusScope {
        id: mainView

        width: parent.width
        height: parent.height
        focus: true

        TabMenu {
            id: tabMenu

            y: 160
            width: parent.width
            height: 160

            keyUpTarget: listMenu
            keyDownTarget: gridMenu

            focus: true
            activeFocusOnTab: true

            onActiveFocusChanged: {
                if (activeFocus)
                    mainView.state = "showTabViews"
            }
        }

        GridMenu {
            id: gridMenu

            y: 320
            width: parent.width
            height: 320
            activeFocusOnTab: true

            keyUpTarget: tabMenu
            keyDownTarget: listMenu
            keyLeftTarget: contextMenu

            onActiveFocusChanged: {
                if (activeFocus)
                    mainView.state = "showGridViews"
            }
        }

        ListMenu {
            id: listMenu

            y: 640
            width: parent.width
            height: 320
            activeFocusOnTab: true

            keyUpTarget: gridMenu
            keyLeftTarget: contextMenu

            onActiveFocusChanged: {
                if (activeFocus)
                    mainView.state = "showListViews"
            }
        }

        Rectangle {
            id: shade
            anchors.fill: parent
            color: "black"
            opacity: 0
        }

        states:  [
            State {
                name: "showTabViews"
                PropertyChanges {
                    tabMenu.y: 160
                    gridMenu.y: 320
                    listMenu.y: 640
                }
            },

            State {
                name: "showGridViews"
                PropertyChanges {
                    tabMenu.y: 0
                    gridMenu.y: 160
                    listMenu.y: 480
                }
            },

            State {
                name: "showListViews"
                PropertyChanges {
                    tabMenu.y: -160
                    gridMenu.y: 0
                    listMenu.y: 320
                }
            }
        ]

        transitions: Transition {
            NumberAnimation {
                properties: "y"
                duration: 600
                easing.type: Easing.OutQuint
            }
        }
    }

    Image {
        source: "images/arrow.png"
        rotation: 90
        anchors.verticalCenter: parent.verticalCenter

        MouseArea {
            anchors.fill: parent
            anchors.margins: -10
            onClicked: contextMenu.focus = true
        }
    }

    ContextMenu {
        keyRightTarget: mainView
        id: contextMenu
        x: -265
        width: 260
        height: parent.height
    }

    states: State {
        name: "contextMenuOpen"
        when: !mainView.activeFocus
        PropertyChanges {
            contextMenu {
                x: 0
                open: true
            }
            mainView.x: 130
            shade.opacity: 0.25
        }
    }

    transitions: Transition {
        NumberAnimation {
            properties: "x,opacity"
            duration: 600
            easing.type: Easing.OutQuint
        }
    }
}
