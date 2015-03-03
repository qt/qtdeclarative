/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
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

            doubleClickSpy.clear()
            mouseDoubleClick(top, 25, 30)
            compare(doubleClickSpy.count, 1)
            compare(doubleClickSpy.signalArguments[0][0], "doubleClick")
        }
    }
}
