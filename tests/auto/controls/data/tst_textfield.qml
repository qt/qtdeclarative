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
    width: 400
    height: 400
    visible: true
    when: windowShown
    name: "TextField"

    Component {
        id: textField
        TextField { }
    }

    function test_creation() {
        var control = textField.createObject(testCase)
        verify(control)
        control.destroy()
    }

    SignalSpy {
        id: pressAndHoldSpy
        signalName: "pressAndHold"
    }

    function test_pressAndHold() {
        var control = textField.createObject(testCase)
        control.width = 200
        pressAndHoldSpy.target = control

        mouseClick(control)
        compare(pressAndHoldSpy.count, 0)
        var interval = Qt.styleHints.mousePressAndHoldInterval

        // Short press duration => nothing happens
        mousePress(control)
        wait(interval * 0.3)
        mouseRelease(control)
        compare(pressAndHoldSpy.count, 0)

        // Long enough press duration => signal emitted
        mousePress(control, 10, 10)
        // Add 20% extra time to allow the control to
        // receive the timer event before we come back here
        wait(interval * 1.2)
        compare(pressAndHoldSpy.count, 1)
        mouseRelease(control)
        compare(pressAndHoldSpy.count, 1)

        // Long enough, but move in between => nothing happens
        pressAndHoldSpy.clear()
        mousePress(control)
        wait(interval * 0.6)
        mouseMove(control, 5, 5, Qt.LeftButton)
        wait(interval * 0.6)
        compare(pressAndHoldSpy.count, 0)
        mouseRelease(control)
        compare(pressAndHoldSpy.count, 0)

        // Long enough, but 2nd press in between => nothing happens
        pressAndHoldSpy.clear()
        mousePress(control, 10, 10)
        wait(interval * 0.6)
        mousePress(control, 10, 10, Qt.RightButton)
        wait(interval * 0.6)
        compare(pressAndHoldSpy.count, 0)
        mouseRelease(control, 10, 10, Qt.LeftButton|Qt.RightButton)
        compare(pressAndHoldSpy.count, 0)

        control.destroy()
    }
}
