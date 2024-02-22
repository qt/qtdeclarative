// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtQuick.Window 2.0
import QtTest 1.1

Rectangle {
    width: 50; height: 50
    id: top
    focus: true

    property bool leftKeyPressed: false
    property bool leftKeyReleased: false

    Keys.onLeftPressed: {
        leftKeyPressed = true
    }

    Keys.onReleased: {
        if (event.key == Qt.Key_Left)
            leftKeyReleased = true
    }

    property bool mouseHasBeenClicked: false

    signal doubleClickSignalHelper(string eventType)

    SignalSpy {
        id: doubleClickSpy
        target: top
        signalName: "doubleClickSignalHelper"
    }

    Window {
        id: sub
        visible: true
        width: 200
        height: 200
        property bool clicked: false
        MouseArea {
            anchors.fill: parent
            onClicked: sub.clicked = true
        }
        Component.onCompleted: show()
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            mouseHasBeenClicked = true
            doubleClickSignalHelper("clicked")
        }
        onPressed: {
            doubleClickSignalHelper("pressed")
        }
        onReleased: {
            doubleClickSignalHelper("released")
        }
        onDoubleClicked: {
            doubleClickSignalHelper("doubleClick")
        }
    }

    TestCase {
        name: "Events"
        when: windowShown       // Must have this line for events to work.

        function test_key_click() {
            skip("test_key_click() is unstable, QTBUG-27671")
            keyClick(Qt.Key_Left)
            tryCompare(top, "leftKeyPressed", true, 10000)
            tryCompare(top, "leftKeyReleased", true, 10000)
        }

        function test_mouse_click() {
            mouseClick(top, 25, 30)
            tryCompare(top, "mouseHasBeenClicked", true, 10000)
        }

        function test_mouse_click_subwindow() {
            compare(sub.width, 200)
            compare(sub.height, 200)
            mouseClick(sub)
            tryCompare(sub, "clicked", true, 10000)
        }

        function test_mouse_doubleclick() {
            doubleClickSpy.clear()
            mouseDoubleClickSequence(top, 25, 30)
            compare(doubleClickSpy.count, 6)
            compare(doubleClickSpy.signalArguments[0][0], "pressed")
            compare(doubleClickSpy.signalArguments[1][0], "released")
            compare(doubleClickSpy.signalArguments[2][0], "clicked")
            compare(doubleClickSpy.signalArguments[3][0], "pressed")
            compare(doubleClickSpy.signalArguments[4][0], "doubleClick")
            compare(doubleClickSpy.signalArguments[5][0], "released")
        }
    }
}
