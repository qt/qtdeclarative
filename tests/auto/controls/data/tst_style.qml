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
    name: "Style"

    Component {
        id: button
        Button { }
    }

    Component {
        id: styledButton
        Button {
            Style.accentColor: "#111111"
            Style.backgroundColor: "#222222"
            Style.baseColor: "#333333"
            Style.focusColor: "#444444"
            Style.frameColor: "#555555"
            Style.pressColor: "#666666"
            Style.selectedTextColor: "#777777"
            Style.selectionColor: "#888888"
            Style.textColor: "#999999"
            Style.padding: 111
            Style.roundness: 222
            Style.spacing: 333
            Style.disabledOpacity: 0.123
        }
    }

    Component {
        id: window
        Window { }
    }

    Component {
        id: styledWindow
        Window {
            Style.accentColor: "#111111"
            Style.backgroundColor: "#222222"
            Style.baseColor: "#333333"
            Style.focusColor: "#444444"
            Style.frameColor: "#555555"
            Style.pressColor: "#666666"
            Style.selectedTextColor: "#777777"
            Style.selectionColor: "#888888"
            Style.textColor: "#999999"
            Style.padding: 111
            Style.roundness: 222
            Style.spacing: 333
            Style.disabledOpacity: 0.123
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
        verify(control.Style)
        verify(control.Style.accentColor !== undefined)
        verify(control.Style.backgroundColor !== undefined)
        verify(control.Style.baseColor !== undefined)
        verify(control.Style.focusColor !== undefined)
        verify(control.Style.frameColor !== undefined)
        verify(control.Style.pressColor !== undefined)
        verify(control.Style.selectedTextColor !== undefined)
        verify(control.Style.selectionColor !== undefined)
        verify(control.Style.textColor !== undefined)
        verify(control.Style.padding !== undefined)
        verify(control.Style.roundness !== undefined)
        verify(control.Style.spacing !== undefined)
        verify(control.Style.disabledOpacity !== undefined)
        control.destroy()
    }

    function test_set() {
        var control = button.createObject(testCase)
        verify(control)
        control.Style.accentColor = "#111111"
        control.Style.backgroundColor = "#222222"
        control.Style.baseColor = "#333333"
        control.Style.focusColor = "#444444"
        control.Style.frameColor = "#555555"
        control.Style.pressColor = "#666666"
        control.Style.selectedTextColor = "#777777"
        control.Style.selectionColor = "#888888"
        control.Style.textColor = "#999999"
        control.Style.padding = 111
        control.Style.roundness = 222
        control.Style.spacing = 333
        control.Style.disabledOpacity = 0.123
        compare(control.Style.accentColor, "#111111")
        compare(control.Style.backgroundColor, "#222222")
        compare(control.Style.baseColor, "#333333")
        compare(control.Style.focusColor, "#444444")
        compare(control.Style.frameColor, "#555555")
        compare(control.Style.pressColor, "#666666")
        compare(control.Style.selectedTextColor, "#777777")
        compare(control.Style.selectionColor, "#888888")
        compare(control.Style.textColor, "#999999")
        compare(control.Style.padding, 111)
        compare(control.Style.roundness, 222)
        compare(control.Style.spacing, 333)
        compare(control.Style.disabledOpacity, 0.123)
        control.destroy()
    }

    function test_reset() {
        var control = styledButton.createObject(testCase)
        verify(control)
        compare(control.Style.accentColor, "#111111")
        compare(control.Style.backgroundColor, "#222222")
        compare(control.Style.baseColor, "#333333")
        compare(control.Style.focusColor, "#444444")
        compare(control.Style.frameColor, "#555555")
        compare(control.Style.pressColor, "#666666")
        compare(control.Style.selectedTextColor, "#777777")
        compare(control.Style.selectionColor, "#888888")
        compare(control.Style.textColor, "#999999")
        compare(control.Style.padding, 111)
        compare(control.Style.roundness, 222)
        compare(control.Style.spacing, 333)
        compare(control.Style.disabledOpacity, 0.123)
        control.Style.accentColor = undefined
        control.Style.backgroundColor = undefined
        control.Style.baseColor = undefined
        control.Style.focusColor = undefined
        control.Style.frameColor = undefined
        control.Style.pressColor = undefined
        control.Style.selectedTextColor = undefined
        control.Style.selectionColor = undefined
        control.Style.textColor = undefined
        control.Style.padding = undefined
        control.Style.roundness = undefined
        control.Style.spacing = undefined
        control.Style.disabledOpacity = undefined
        compare(control.Style.accentColor, testCase.Style.accentColor)
        compare(control.Style.backgroundColor, testCase.Style.backgroundColor)
        compare(control.Style.baseColor, testCase.Style.baseColor)
        compare(control.Style.focusColor, testCase.Style.focusColor)
        compare(control.Style.frameColor, testCase.Style.frameColor)
        compare(control.Style.pressColor, testCase.Style.pressColor)
        compare(control.Style.selectedTextColor, testCase.Style.selectedTextColor)
        compare(control.Style.selectionColor, testCase.Style.selectionColor)
        compare(control.Style.textColor, testCase.Style.textColor)
        compare(control.Style.padding, testCase.Style.padding)
        compare(control.Style.roundness, testCase.Style.roundness)
        compare(control.Style.spacing, testCase.Style.spacing)
        compare(control.Style.disabledOpacity, testCase.Style.disabledOpacity)
        control.destroy()
    }

    function test_inheritance_data() {
        return [
            { tag: "accentColor", value1: "#111111", value2: "#101010" },
            { tag: "backgroundColor", value1: "#222222", value2: "#202020" },
            { tag: "baseColor", value1: "#333333", value2: "#303030" },
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
        parent.Style[prop] = data.value1
        compare(parent.Style[prop], data.value1)

        var child1 = button.createObject(parent)
        compare(child1.Style[prop], data.value1)

        parent.Style[prop] = data.value2
        compare(parent.Style[prop], data.value2)
        compare(child1.Style[prop], data.value2)

        var child2 = button.createObject(parent)
        compare(child2.Style[prop], data.value2)

        child2.Style[prop] = data.value1
        compare(child2.Style[prop], data.value1)
        compare(child1.Style[prop], data.value2)
        compare(parent.Style[prop], data.value2)

        parent.Style[prop] = undefined
        verify(parent.Style[prop] !== data.value1)
        verify(parent.Style[prop] !== data.value2)
        verify(parent.Style[prop] !== undefined)
        compare(child1.Style[prop], parent.Style[prop])
        verify(child2.Style[prop] !== parent.Style[prop])

        var grandChild1 = button.createObject(child1)
        var grandChild2 = button.createObject(child2)
        compare(grandChild1.Style[prop], child1.Style[prop])
        compare(grandChild2.Style[prop], child2.Style[prop])

        var stylelessGrandGrandChild = button.createObject(grandChild1)
        var grandGrandGrandChild1 = button.createObject(stylelessGrandGrandChild)
        compare(grandGrandGrandChild1.Style[prop], parent.Style[prop])

        child1.Style[prop] = data.value2
        verify(parent.Style[prop] !== data.value2)
        compare(child1.Style[prop], data.value2)
        compare(grandChild1.Style[prop], data.value2)
        compare(grandGrandGrandChild1.Style[prop], data.value2)

        parent.destroy()
    }

    function test_window() {
        var parent = window.createObject()

        var control = button.createObject(parent.contentItem)
        compare(control.Style.backgroundColor, parent.Style.backgroundColor)

        var styledChild = styledWindow.createObject(window)
        verify(styledChild.Style.backgroundColor !== parent.Style.backgroundColor)

        var unstyledChild = window.createObject(window)
        compare(unstyledChild.Style.backgroundColor, parent.Style.backgroundColor)

        parent.Style.backgroundColor = "#123456"
        compare(control.Style.backgroundColor, "#123456")
        verify(styledChild.Style.backgroundColor !== "#123456")
        // ### FIXME: compare(unstyledChild.Style.backgroundColor, "#123456")

        parent.destroy()
    }

    function test_loader() {
        var control = loader.createObject(testCase)
        control.Style.accentColor = "#111111"
        control.active = true
        compare(control.item.Style.accentColor, "#111111")
        control.Style.accentColor = "#222222"
        compare(control.item.Style.accentColor, "#222222")
        control.active = false
        control.Style.accentColor = "#333333"
        control.active = true
        compare(control.item.Style.accentColor, "#333333")
        control.destroy()
    }
}
