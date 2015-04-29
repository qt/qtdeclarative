/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.2
import QtTest 1.0
import QtQuick.Controls 2.0

TestCase {
    id: testCase
    width: 200
    height: 200
    visible: true
    when: windowShown
    name: "Button"

    SignalSpy {
        id: pressedChangedSpy
        signalName: "pressedChanged"
    }

    SignalSpy {
        id: releasedSpy
        signalName: "released"
    }

    SignalSpy {
        id: canceledSpy
        signalName: "canceled"
    }

    SignalSpy {
        id: clickedSpy
        signalName: "clicked"
    }

    Component {
        id: button
        Button { }
    }

    function init() {
        verify(!pressedChangedSpy.target)
        verify(!releasedSpy.target)
        verify(!clickedSpy.target)
        compare(pressedChangedSpy.count, 0)
        compare(releasedSpy.count, 0)
        compare(clickedSpy.count, 0)
    }

    function cleanup() {
        pressedChangedSpy.target = null
        releasedSpy.target = null
        clickedSpy.target = null
        pressedChangedSpy.clear()
        releasedSpy.clear()
        clickedSpy.clear()
    }

    function test_defaults() {
        var control = button.createObject(testCase)
        verify(control)
        verify(control.label)
        compare(control.text, "")
        compare(control.pressed, false)
        control.destroy()
    }

    function test_text() {
        var control = button.createObject(testCase)
        compare(control.text, "")
        control.text = "Button"
        compare(control.text, "Button")
        control.text = ""
        compare(control.text, "")
        control.destroy()
    }

    function test_mouse() {
        var control = button.createObject(testCase)

        pressedChangedSpy.target = control
        releasedSpy.target = control
        canceledSpy.target = control
        clickedSpy.target = control
        verify(pressedChangedSpy.valid)
        verify(releasedSpy.valid)
        verify(canceledSpy.valid)
        verify(clickedSpy.valid)

        // check
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(pressedChangedSpy.count, 1)
        compare(releasedSpy.count, 0)
        compare(canceledSpy.count, 0)
        compare(clickedSpy.count, 0)
        compare(control.pressed, true)

        mouseRelease(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(pressedChangedSpy.count, 2)
        compare(releasedSpy.count, 1)
        compare(canceledSpy.count, 0)
        compare(clickedSpy.count, 1)
        compare(control.pressed, false)

        // uncheck
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(pressedChangedSpy.count, 3)
        compare(releasedSpy.count, 1)
        compare(canceledSpy.count, 0)
        compare(clickedSpy.count, 1)
        compare(control.pressed, true)

        mouseRelease(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(pressedChangedSpy.count, 4)
        compare(releasedSpy.count, 2)
        compare(canceledSpy.count, 0)
        compare(clickedSpy.count, 2)
        compare(control.pressed, false)

        // release outside
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(pressedChangedSpy.count, 5)
        compare(releasedSpy.count, 2)
        compare(canceledSpy.count, 0)
        compare(clickedSpy.count, 2)
        compare(control.pressed, true)

        mouseMove(control, control.width * 2, control.height * 2, 0, Qt.LeftButton)
        compare(control.pressed, false)

        mouseRelease(control, control.width * 2, control.height * 2, Qt.LeftButton)
        compare(pressedChangedSpy.count, 6)
        compare(releasedSpy.count, 2)
        compare(canceledSpy.count, 1)
        compare(clickedSpy.count, 2)
        compare(control.pressed, false)

        // right button
        mousePress(control, control.width / 2, control.height / 2, Qt.RightButton)
        compare(pressedChangedSpy.count, 6)
        compare(releasedSpy.count, 2)
        compare(canceledSpy.count, 1)
        compare(clickedSpy.count, 2)
        compare(control.pressed, false)

        mouseRelease(control, control.width / 2, control.height / 2, Qt.RightButton)
        compare(pressedChangedSpy.count, 6)
        compare(releasedSpy.count, 2)
        compare(canceledSpy.count, 1)
        compare(clickedSpy.count, 2)
        compare(control.pressed, false)

        control.destroy()
    }

    function test_keys() {
        var control = button.createObject(testCase)

        clickedSpy.target = control
        verify(clickedSpy.valid)

        control.forceActiveFocus()
        verify(control.activeFocus)

        // check
        keyClick(Qt.Key_Space)
        compare(clickedSpy.count, 1)

        // uncheck
        keyClick(Qt.Key_Space)
        compare(clickedSpy.count, 2)

        // no change
        var keys = [Qt.Key_Enter, Qt.Key_Return, Qt.Key_Escape, Qt.Key_Tab]
        for (var i = 0; i < keys.length; ++i) {
            keyClick(keys[i])
            compare(clickedSpy.count, 2)
        }

        control.destroy()
    }
}
