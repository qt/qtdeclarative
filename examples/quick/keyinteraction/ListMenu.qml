// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQml
import QtQuick

FocusScope {
    id: menu

    clip: true
    required property Item keyUpTarget
    required property Item keyLeftTarget

    ListView {
        id: list1

        y: activeFocus ? 10 : 40
        width: parent.width / 3
        height: parent.height - 20
        focus: true

        KeyNavigation.up: menu.keyUpTarget
        KeyNavigation.left: menu.keyLeftTarget
        KeyNavigation.right: list2

        model: 10
        cacheBuffer: 200
        delegate: ListViewDelegate {}

        Behavior on y {
            NumberAnimation {
                duration: 600
                easing.type: Easing.OutQuint
            }
        }
    }

    ListView {
        id: list2

        y: activeFocus ? 10 : 40
        x: parseInt(parent.width / 3)
        width: parent.width / 3
        height: parent.height - 20

        KeyNavigation.up: menu.keyUpTarget
        KeyNavigation.left: list1
        KeyNavigation.right: list3

        model: 10
        cacheBuffer: 200
        delegate: ListViewDelegate {}

        Behavior on y {
            NumberAnimation {
                duration: 600
                easing.type: Easing.OutQuint
            }
        }
    }

    ListView {
        id: list3

        y: activeFocus ? 10 : 40
        x: parseInt(2 * parent.width / 3)
        width: parent.width / 3
        height: parent.height - 20

        KeyNavigation.up: menu.keyUpTarget
        KeyNavigation.left: list2

        model: 10
        cacheBuffer: 200
        delegate: ListViewDelegate {}

        Behavior on y {
            NumberAnimation {
                duration: 600
                easing.type: Easing.OutQuint
            }
        }
    }

    Rectangle {
        width: parent.width
        height: 1
        color: "#D1DBBD"
    }

    Rectangle {
        y: 1
        width: parent.width
        height: 10
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: "#3E606F"
            }
            GradientStop {
                position: 1.0
                color: "transparent"
            }
        }
    }

    Rectangle {
        y: parent.height - 10
        width: parent.width
        height: 10
        gradient: Gradient {
            GradientStop {
                position: 1.0
                color: "#3E606F"
            }
            GradientStop {
                position: 0.0
                color: "transparent"
            }
        }
    }
}
