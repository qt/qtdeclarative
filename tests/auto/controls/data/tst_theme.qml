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
import QtQuick.Window 2.2
import QtTest 1.0
import QtQuick.Controls 2.0

TestCase {
    id: testCase
    width: 200
    height: 200
    visible: true
    when: windowShown
    name: "Theme"

    Component {
        id: button
        Button { }
    }

    Component {
        id: themedButton
        Button {
            Theme.accentColor: "#111111"
            Theme.backgroundColor: "#222222"
            Theme.baseColor: "#333333"
            Theme.disabledColor: "#aaaaaa"
            Theme.focusColor: "#444444"
            Theme.frameColor: "#555555"
            Theme.pressColor: "#666666"
            Theme.selectedTextColor: "#777777"
            Theme.selectionColor: "#888888"
            Theme.textColor: "#999999"
            Theme.padding: 111
            Theme.roundness: 222
            Theme.spacing: 333
            Theme.disabledOpacity: 0.123
        }
    }

    Component {
        id: window
        Window { }
    }

    Component {
        id: themedWindow
        Window {
            Theme.accentColor: "#111111"
            Theme.backgroundColor: "#222222"
            Theme.baseColor: "#333333"
            Theme.disabledColor: "#aaaaaa"
            Theme.focusColor: "#444444"
            Theme.frameColor: "#555555"
            Theme.pressColor: "#666666"
            Theme.selectedTextColor: "#777777"
            Theme.selectionColor: "#888888"
            Theme.textColor: "#999999"
            Theme.padding: 111
            Theme.roundness: 222
            Theme.spacing: 333
            Theme.disabledOpacity: 0.123
        }
    }

    Component {
        id: loader
        Loader {
            active: false
            sourceComponent: Button { }
        }
    }

    function test_defaults() {
        var control = button.createObject(testCase)
        verify(control)
        verify(control.Theme)
        verify(control.Theme.accentColor !== undefined)
        verify(control.Theme.backgroundColor !== undefined)
        verify(control.Theme.baseColor !== undefined)
        verify(control.Theme.disabledColor !== undefined)
        verify(control.Theme.focusColor !== undefined)
        verify(control.Theme.frameColor !== undefined)
        verify(control.Theme.pressColor !== undefined)
        verify(control.Theme.selectedTextColor !== undefined)
        verify(control.Theme.selectionColor !== undefined)
        verify(control.Theme.textColor !== undefined)
        verify(control.Theme.padding !== undefined)
        verify(control.Theme.roundness !== undefined)
        verify(control.Theme.spacing !== undefined)
        verify(control.Theme.disabledOpacity !== undefined)
        control.destroy()
    }

    function test_set() {
        var control = button.createObject(testCase)
        verify(control)
        control.Theme.accentColor = "#111111"
        control.Theme.backgroundColor = "#222222"
        control.Theme.baseColor = "#333333"
        control.Theme.disabledColor = "#aaaaaa"
        control.Theme.focusColor = "#444444"
        control.Theme.frameColor = "#555555"
        control.Theme.pressColor = "#666666"
        control.Theme.selectedTextColor = "#777777"
        control.Theme.selectionColor = "#888888"
        control.Theme.textColor = "#999999"
        control.Theme.padding = 111
        control.Theme.roundness = 222
        control.Theme.spacing = 333
        control.Theme.disabledOpacity = 0.123
        compare(control.Theme.accentColor, "#111111")
        compare(control.Theme.backgroundColor, "#222222")
        compare(control.Theme.baseColor, "#333333")
        compare(control.Theme.disabledColor, "#aaaaaa")
        compare(control.Theme.focusColor, "#444444")
        compare(control.Theme.frameColor, "#555555")
        compare(control.Theme.pressColor, "#666666")
        compare(control.Theme.selectedTextColor, "#777777")
        compare(control.Theme.selectionColor, "#888888")
        compare(control.Theme.textColor, "#999999")
        compare(control.Theme.padding, 111)
        compare(control.Theme.roundness, 222)
        compare(control.Theme.spacing, 333)
        compare(control.Theme.disabledOpacity, 0.123)
        control.destroy()
    }

    function test_reset() {
        var control = themedButton.createObject(testCase)
        verify(control)
        compare(control.Theme.accentColor, "#111111")
        compare(control.Theme.backgroundColor, "#222222")
        compare(control.Theme.baseColor, "#333333")
        compare(control.Theme.disabledColor, "#aaaaaa")
        compare(control.Theme.focusColor, "#444444")
        compare(control.Theme.frameColor, "#555555")
        compare(control.Theme.pressColor, "#666666")
        compare(control.Theme.selectedTextColor, "#777777")
        compare(control.Theme.selectionColor, "#888888")
        compare(control.Theme.textColor, "#999999")
        compare(control.Theme.padding, 111)
        compare(control.Theme.roundness, 222)
        compare(control.Theme.spacing, 333)
        compare(control.Theme.disabledOpacity, 0.123)
        control.Theme.accentColor = undefined
        control.Theme.backgroundColor = undefined
        control.Theme.baseColor = undefined
        control.Theme.disabledColor = undefined
        control.Theme.focusColor = undefined
        control.Theme.frameColor = undefined
        control.Theme.pressColor = undefined
        control.Theme.selectedTextColor = undefined
        control.Theme.selectionColor = undefined
        control.Theme.textColor = undefined
        control.Theme.padding = undefined
        control.Theme.roundness = undefined
        control.Theme.spacing = undefined
        control.Theme.disabledOpacity = undefined
        compare(control.Theme.accentColor, testCase.Theme.accentColor)
        compare(control.Theme.backgroundColor, testCase.Theme.backgroundColor)
        compare(control.Theme.baseColor, testCase.Theme.baseColor)
        compare(control.Theme.disabledColor, testCase.Theme.disabledColor)
        compare(control.Theme.focusColor, testCase.Theme.focusColor)
        compare(control.Theme.frameColor, testCase.Theme.frameColor)
        compare(control.Theme.pressColor, testCase.Theme.pressColor)
        compare(control.Theme.selectedTextColor, testCase.Theme.selectedTextColor)
        compare(control.Theme.selectionColor, testCase.Theme.selectionColor)
        compare(control.Theme.textColor, testCase.Theme.textColor)
        compare(control.Theme.padding, testCase.Theme.padding)
        compare(control.Theme.roundness, testCase.Theme.roundness)
        compare(control.Theme.spacing, testCase.Theme.spacing)
        compare(control.Theme.disabledOpacity, testCase.Theme.disabledOpacity)
        control.destroy()
    }

    function test_inheritance_data() {
        return [
            { tag: "accentColor", value1: "#111111", value2: "#101010" },
            { tag: "backgroundColor", value1: "#222222", value2: "#202020" },
            { tag: "baseColor", value1: "#333333", value2: "#303030" },
            { tag: "disabledColor", value1: "#aaaaaa", value2: "#a0a0a0" },
            { tag: "focusColor", value1: "#444444", value2: "#404040" },
            { tag: "frameColor", value1: "#555555", value2: "#505050" },
            { tag: "pressColor", value1: "#666666", value2: "#606060" },
            { tag: "selectedTextColor", value1: "#777777", value2: "#707070" },
            { tag: "selectionColor", value1: "#888888", value2: "#808080" },
            { tag: "textColor", value1: "#999999", value2: "#909090" },
            { tag: "padding", value1: 11, value2: 10 },
            { tag: "roundness", value1: 22, value2: 20 },
            { tag: "spacing", value1: 33, value2: 30 },
            { tag: "disabledOpacity", value1: 0.123, value2: 0.345 }
        ]
    }

    function test_inheritance(data) {
        var prop = data.tag
        var parent = button.createObject(testCase)
        parent.Theme[prop] = data.value1
        compare(parent.Theme[prop], data.value1)

        var child1 = button.createObject(parent)
        compare(child1.Theme[prop], data.value1)

        parent.Theme[prop] = data.value2
        compare(parent.Theme[prop], data.value2)
        compare(child1.Theme[prop], data.value2)

        var child2 = button.createObject(parent)
        compare(child2.Theme[prop], data.value2)

        child2.Theme[prop] = data.value1
        compare(child2.Theme[prop], data.value1)
        compare(child1.Theme[prop], data.value2)
        compare(parent.Theme[prop], data.value2)

        parent.Theme[prop] = undefined
        verify(parent.Theme[prop] !== data.value1)
        verify(parent.Theme[prop] !== data.value2)
        verify(parent.Theme[prop] !== undefined)
        compare(child1.Theme[prop], parent.Theme[prop])
        verify(child2.Theme[prop] !== parent.Theme[prop])

        var grandChild1 = button.createObject(child1)
        var grandChild2 = button.createObject(child2)
        compare(grandChild1.Theme[prop], child1.Theme[prop])
        compare(grandChild2.Theme[prop], child2.Theme[prop])

        var themelessGrandGrandChild = button.createObject(grandChild1)
        var grandGrandGrandChild1 = button.createObject(themelessGrandGrandChild)
        compare(grandGrandGrandChild1.Theme[prop], parent.Theme[prop])

        child1.Theme[prop] = data.value2
        verify(parent.Theme[prop] !== data.value2)
        compare(child1.Theme[prop], data.value2)
        compare(grandChild1.Theme[prop], data.value2)
        compare(grandGrandGrandChild1.Theme[prop], data.value2)

        parent.destroy()
    }

    function test_window() {
        var parent = window.createObject()

        var control = button.createObject(parent.contentItem)
        compare(control.Theme.backgroundColor, parent.Theme.backgroundColor)

        var themedChild = themedWindow.createObject(window)
        verify(themedChild.Theme.backgroundColor !== parent.Theme.backgroundColor)

        var unthemedChild = window.createObject(window)
        compare(unthemedChild.Theme.backgroundColor, parent.Theme.backgroundColor)

        parent.Theme.backgroundColor = "#123456"
        compare(control.Theme.backgroundColor, "#123456")
        verify(themedChild.Theme.backgroundColor !== "#123456")
        // ### FIXME: compare(unthemedChild.Theme.backgroundColor, "#123456")

        parent.destroy()
    }

    function test_loader() {
        var control = loader.createObject(testCase)
        control.Theme.accentColor = "#111111"
        control.active = true
        compare(control.item.Theme.accentColor, "#111111")
        control.Theme.accentColor = "#222222"
        compare(control.item.Theme.accentColor, "#222222")
        control.active = false
        control.Theme.accentColor = "#333333"
        control.active = true
        compare(control.item.Theme.accentColor, "#333333")
        control.destroy()
    }
}
