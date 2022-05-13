// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QtTest 1.1

Rectangle {
    id: screen
    width: 320; height: 240
    property string text
    property bool changeColor: false

    Text { id: s1; text: "Hello" }
    Rectangle { id: r1; width: 1; height: 1; color: "yellow" }
    Rectangle { id: r2; width: 1; height: 1; color: "red" }

    Binding { target: screen; property: "text"; value: s1.text }
    Binding { target: screen; property: "color"; value: r1.color }
    Binding { target: screen; property: "color"; value: r2.color; when: screen.changeColor == true }

    TestCase {
        name: "Binding2"

        function test_binding2() {
            compare(screen.color, "#ffff00")    // Yellow
            compare(screen.text, "Hello")

            screen.changeColor = true
            compare(screen.color, "#ff0000")    // Red
        }
    }
}
