/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

import QtQuick 2.4
import QtTest 1.0
import QtQuick.Controls 2.0

TestCase {
    id: testCase
    width: 400
    height: 400
    visible: true
    when: windowShown
    name: "ToolTip"

    Component {
        id: toolTip
        ToolTip { }
    }

    Component {
        id: mouseArea
        MouseArea { }
    }

    Component {
        id: signalSpy
        SignalSpy { }
    }

    QtObject {
        id: object
    }

    function test_properties_data() {
        return [
            {tag: "text", property: "text", defaultValue: "", setValue: "Hello", signalName: "textChanged"},
            {tag: "delay", property: "delay", defaultValue: 0, setValue: 1000, signalName: "delayChanged"},
            {tag: "timeout", property: "timeout", defaultValue: -1, setValue: 2000, signalName: "timeoutChanged"}
        ]
    }

    function test_properties(data) {
        var control = toolTip.createObject(testCase)
        verify(control)

        compare(control[data.property], data.defaultValue)

        var spy = signalSpy.createObject(testCase, {target: control, signalName: data.signalName})
        verify(spy.valid)

        control[data.property] = data.setValue
        compare(control[data.property], data.setValue)
        compare(spy.count, 1)

        spy.destroy()
        control.destroy()
    }

    function test_attached_data() {
        return [
            {tag: "text", property: "text", defaultValue: "", setValue: "Hello", signalName: "textChanged"},
            {tag: "delay", property: "delay", defaultValue: 0, setValue: 1000, signalName: "delayChanged"},
            {tag: "timeout", property: "timeout", defaultValue: -1, setValue: 2000, signalName: "timeoutChanged"}
        ]
    }

    function test_attached(data) {
        var item1 = mouseArea.createObject(testCase)
        verify(item1)

        var item2 = mouseArea.createObject(testCase)
        verify(item2)

        compare(item1.ToolTip[data.property], data.defaultValue)
        compare(item2.ToolTip[data.property], data.defaultValue)

        var spy1 = signalSpy.createObject(item1, {target: item1.ToolTip, signalName: data.signalName})
        verify(spy1.valid)

        var spy2 = signalSpy.createObject(item2, {target: item2.ToolTip, signalName: data.signalName})
        verify(spy2.valid)

        item1.ToolTip[data.property] = data.setValue
        compare(item1.ToolTip[data.property], data.setValue)
        compare(spy1.count, 1)

        compare(spy2.count, 0)
        compare(item2.ToolTip[data.property], data.defaultValue)

        item1.destroy()
        item2.destroy()
    }

    function test_delay_data() {
        return [
            {tag: "0", delay: 0},
            {tag: "100", delay: 100},
        ]
    }

    function test_delay(data) {
        var control = toolTip.createObject(testCase, {delay: data.delay})

        compare(control.visible, false)
        control.open()
        compare(control.visible, data.delay <= 0)
        tryCompare(control, "visible", true)

        control.destroy()
    }

    function test_timeout() {
        var control = toolTip.createObject(testCase, {timeout: 100})

        compare(control.visible, false)
        control.open()
        compare(control.visible, true)
        tryCompare(control, "visible", false)

        control.destroy()
    }

    function test_warning() {
        ignoreWarning(Qt.resolvedUrl("tst_tooltip.qml") + ":68:5: QML QtObject: ToolTip must be attached to an Item")
        object.ToolTip.text = ""
    }
}
