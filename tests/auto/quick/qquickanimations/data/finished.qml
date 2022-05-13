// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.12

Item {
    id: root
    width: 400
    height: 400

    property bool finishedUsableInQml: false

    property alias simpleTopLevelAnimation: simpleTopLevelAnimation
    property real foo: 0

    property alias transitionRect: transitionRect
    property alias transition: transition
    property alias animationWithinTransition: animationWithinTransition

    property real bar: 0
    property alias animationWithinBehavior: animationWithinBehavior
    property alias behavior: behavior

    NumberAnimation {
        id: simpleTopLevelAnimation
        target: root
        property: "foo"
        from: 0
        to: 1
        duration: 10

        onFinished: finishedUsableInQml = true
    }

    Rectangle {
        id: transitionRect
        color: "green"
        width: 50
        height: 50
        anchors.centerIn: parent

        states: State {
            name: "go"
        }
        transitions: Transition {
            id: transition
            to: "go"
            SequentialAnimation {
                NumberAnimation {
                    id: animationWithinTransition
                    duration: 10
                    property: "foo"
                    from: 1
                    to: 2
                }
            }
        }
    }

    Behavior on bar {
        id: behavior
        NumberAnimation {
            id: animationWithinBehavior
            duration: 10
            property: "bar"
            from: 0
            to: 1
        }
    }
}
