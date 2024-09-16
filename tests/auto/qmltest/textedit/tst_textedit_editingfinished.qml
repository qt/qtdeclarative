// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.6
import QtTest 1.1

Row {
    width: 100
    height: 50
    spacing: 10

    property alias control1: _control1
    property alias control2: _control2
    TextEdit {
        id: _control1
        text: 'A'
        property bool myeditingfinished: false
        onEditingFinished: myeditingfinished = true
        activeFocusOnTab: true
    }
    TextEdit {
        id: _control2
        text: 'B'
        property bool myeditingfinished: false
        onEditingFinished: myeditingfinished = true
        activeFocusOnTab: true
    }

    TestCase {
        name: "TextEdit_editingFinished"
        when: windowShown

        function test_editingFinished() {
            control1.forceActiveFocus()
            verify(control1.activeFocus)
            verify(!control2.activeFocus)

            verify(control1.myeditingfinished === false)
            verify(control2.myeditingfinished === false)

            keyClick(Qt.Key_Backtab)
            verify(!control1.activeFocus)
            verify(control2.activeFocus)
            verify(control1.myeditingfinished === true)

            keyClick(Qt.Key_Backtab)
            verify(control1.activeFocus)
            verify(!control2.activeFocus)
            verify(control2.myeditingfinished === true)
        }
    }
}
