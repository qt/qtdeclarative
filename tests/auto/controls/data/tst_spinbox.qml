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
import Qt.labs.controls 1.0

TestCase {
    id: testCase
    width: 400
    height: 400
    visible: true
    when: windowShown
    name: "SpinBox"

    SignalSpy{
        id: upPressedSpy
        signalName: "pressedChanged"
    }

    SignalSpy{
        id: downPressedSpy
        signalName: "pressedChanged"
    }

    Component {
        id: spinBox
        SpinBox { }
    }

    function init() {
        verify(!upPressedSpy.target)
        compare(upPressedSpy.count, 0)
        verify(!downPressedSpy.target)
        compare(downPressedSpy.count, 0)
    }

    function cleanup() {
        upPressedSpy.target = null
        upPressedSpy.clear()
        downPressedSpy.target = null
        downPressedSpy.clear()
    }

    function test_defaults() {
        var control = spinBox.createObject(testCase)
        verify(control)

        compare(control.from, 0)
        compare(control.to, 99)
        compare(control.value, 0)
        compare(control.stepSize, 1)
        compare(control.up.pressed, false)
        compare(control.down.pressed, false)

        control.destroy()
    }

    function test_value() {
        var control = spinBox.createObject(testCase)
        verify(control)

        compare(control.value, 0)
        control.value = 50
        compare(control.value, 50)
        control.value = 99
        compare(control.value, 99)
        control.value = -99
        compare(control.value, 0)
        control.value = 100
        compare(control.value, 99)

        control.destroy()
    }

    function test_range() {
        var control = spinBox.createObject(testCase, {from: 0, to: 100, value: 50})
        verify(control)

        compare(control.from, 0)
        compare(control.to, 100)
        compare(control.value, 50)

        control.value = 1000
        compare(control.value, 100)

        control.value = -1
        compare(control.value, 0)

        control.from = 25
        compare(control.from, 25)
        compare(control.value, 25)

        control.to = 75
        compare(control.to, 75)
        compare(control.value, 25)

        control.value = 50
        compare(control.value, 50)

        control.destroy()
    }

    function test_inverted() {
        var control = spinBox.createObject(testCase, {from: 100, to: -100})
        verify(control)

        compare(control.from, 100)
        compare(control.to, -100)
        compare(control.value, 0)

        control.value = 200
        compare(control.value, 100)

        control.value = -200
        compare(control.value, -100)

        control.value = 0
        compare(control.value, 0)

        control.destroy()
    }

    function test_mouse() {
        var control = spinBox.createObject(testCase, {stepSize: 50})
        verify(control)

        upPressedSpy.target = control.up
        verify(upPressedSpy.valid)

        mousePress(control.up.indicator)
        compare(upPressedSpy.count, 1)
        compare(control.up.pressed, true)
        compare(downPressedSpy.count, 0)
        compare(control.down.pressed, false)
        compare(control.value, 0)

        mouseRelease(control.up.indicator)
        compare(upPressedSpy.count, 2)
        compare(control.up.pressed, false)
        compare(downPressedSpy.count, 0)
        compare(control.down.pressed, false)
        compare(control.value, 50)

        downPressedSpy.target = control.down
        verify(downPressedSpy.valid)

        mousePress(control.down.indicator)
        compare(downPressedSpy.count, 1)
        compare(control.down.pressed, true)
        compare(upPressedSpy.count, 2)
        compare(control.up.pressed, false)
        compare(control.value, 50)

        mouseRelease(control.down.indicator)
        compare(downPressedSpy.count, 2)
        compare(control.down.pressed, false)
        compare(upPressedSpy.count, 2)
        compare(control.up.pressed, false)
        compare(control.value, 0)

        control.destroy()
    }

    function test_keys() {
        var control = spinBox.createObject(testCase)
        verify(control)

        var upPressedCount = 0
        var downPressedCount = 0

        upPressedSpy.target = control.up
        verify(upPressedSpy.valid)

        downPressedSpy.target = control.down
        verify(downPressedSpy.valid)

        control.forceActiveFocus()
        verify(control.activeFocus)

        control.value = 50
        compare(control.value, 50)

        for (var d1 = 1; d1 <= 10; ++d1) {
            keyPress(Qt.Key_Down)
            compare(control.down.pressed, true)
            compare(control.up.pressed, false)
            compare(downPressedSpy.count, ++downPressedCount)

            compare(control.value, 50 - d1)

            keyRelease(Qt.Key_Down)
            compare(control.down.pressed, false)
            compare(control.up.pressed, false)
            compare(downPressedSpy.count, ++downPressedCount)
        }
        compare(control.value, 40)

        for (var i1 = 1; i1 <= 10; ++i1) {
            keyPress(Qt.Key_Up)
            compare(control.up.pressed, true)
            compare(control.down.pressed, false)
            compare(upPressedSpy.count, ++upPressedCount)

            compare(control.value, 40 + i1)

            keyRelease(Qt.Key_Up)
            compare(control.down.pressed, false)
            compare(control.up.pressed, false)
            compare(upPressedSpy.count, ++upPressedCount)
        }
        compare(control.value, 50)

        control.stepSize = 25
        compare(control.stepSize, 25)

        for (var d2 = 1; d2 <= 10; ++d2) {
            keyPress(Qt.Key_Down)
            compare(control.down.pressed, true)
            compare(control.up.pressed, false)
            compare(downPressedSpy.count, ++downPressedCount)

            compare(control.value, Math.max(0, 50 - d2 * 25))

            keyRelease(Qt.Key_Down)
            compare(control.down.pressed, false)
            compare(control.up.pressed, false)
            compare(downPressedSpy.count, ++downPressedCount)
        }
        compare(control.value, 0)

        for (var i2 = 1; i2 <= 10; ++i2) {
            keyPress(Qt.Key_Up)
            compare(control.up.pressed, true)
            compare(control.down.pressed, false)
            compare(upPressedSpy.count, ++upPressedCount)

            compare(control.value, Math.min(99, i2 * 25))

            keyRelease(Qt.Key_Up)
            compare(control.down.pressed, false)
            compare(control.up.pressed, false)
            compare(upPressedSpy.count, ++upPressedCount)
        }
        compare(control.value, 99)

        control.destroy()
    }

    function test_locale() {
        var control = spinBox.createObject(testCase)
        verify(control)

        control.locale = Qt.locale("ar_EG") // Arabic, Egypt

        var numbers = ["٠", "١", "٢", "٣", "٤", "٥", "٦", "٧", "٨", "٩"]
        for (var i = 0; i < 10; ++i) {
            control.value = i
            compare(control.contentItem.text, numbers[i])
        }

        control.destroy()
    }

    function test_baseline() {
        var control = spinBox.createObject(testCase)
        verify(control)
        compare(control.baselineOffset, control.contentItem.y + control.contentItem.baselineOffset)
        control.destroy()
    }

    function test_focus() {
        var control = spinBox.createObject(testCase, {from: 10, to: 1000, value: 100, focus: true})
        verify(control)

        control.forceActiveFocus()
        compare(control.activeFocus, true)

        compare(control.from, 10)
        compare(control.to, 1000)
        compare(control.value, 100)

        control.focus = false
        compare(control.activeFocus, false)

        compare(control.from, 10)
        compare(control.to, 1000)
        compare(control.value, 100)

        control.destroy()
    }
}
