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
    name: "StackView"

    Item { id: item  }
    TextField { id: textField }
    Component { id: component; Item { } }

    Component {
        id: stackView
        StackView { }
    }

    function test_defaults() {
        var control = stackView.createObject(testCase)
        verify(control)
        verify(control.delegate)
        compare(control.depth, 0)
        compare(control.busy, false)
        compare(control.currentItem, null)
        compare(control.initialItem, null)
        control.destroy()
    }

    function test_initialItem() {
        var control1 = stackView.createObject(testCase, {initialItem: item})
        compare(control1.currentItem, item)
        control1.destroy()

        var control2 = stackView.createObject(testCase, {initialItem: component})
        verify(control2.currentItem)
        control2.destroy()
    }

    function test_currentItem() {
        var control = stackView.createObject(testCase, {initialItem: item})
        compare(control.currentItem, item)
        control.push(component)
        verify(control.currentItem !== item)
        control.pop({immediate: true})
        compare(control.currentItem, item)
        control.destroy()
    }

    function test_busy() {
        var control = stackView.createObject(testCase)
        compare(control.busy, false)
        control.push(component)
        compare(control.busy, false)
        control.push(component)
        compare(control.busy, true)
        tryCompare(control, "busy", false)
        control.pop()
        compare(control.busy, true)
        tryCompare(control, "busy", false)
        control.destroy()
    }

    function test_status() {
        var control = stackView.createObject(testCase)

        var item1 = component.createObject(control)
        compare(item1.Stack.status, Stack.Inactive)
        control.push(item1)
        compare(item1.Stack.status, Stack.Active)

        var item2 = component.createObject(control)
        compare(item2.Stack.status, Stack.Inactive)
        control.push(item2)
        compare(item2.Stack.status, Stack.Activating)
        compare(item1.Stack.status, Stack.Deactivating)
        tryCompare(item2.Stack, "status", Stack.Active)
        tryCompare(item1.Stack, "status", Stack.Inactive)

        control.pop()
        compare(item2.Stack.status, Stack.Deactivating)
        compare(item1.Stack.status, Stack.Activating)
        tryCompare(item2.Stack, "status", Stack.Inactive)
        tryCompare(item1.Stack, "status", Stack.Active)

        control.destroy()
    }

    function test_depth() {
        var control = stackView.createObject(testCase)
        compare(control.depth, 0)
        control.push(item)
        compare(control.depth, 1)
        control.push(item)
        compare(control.depth, 2)
        control.pop()
        compare(control.depth, 1)
        control.push(component)
        compare(control.depth, 2)
        control.pop()
        compare(control.depth, 1)
        control.pop() // ignored
        compare(control.depth, 1)
        control.clear()
        compare(control.depth, 0)
        control.destroy()
    }

    function test_size() {
        var container = component.createObject(testCase, {width: 200, height: 200})
        var control = stackView.createObject(container, {width: 100, height: 100})
        container.width += 10
        container.height += 20
        compare(control.width, 100)
        compare(control.height, 100)
        container.destroy()
    }

    function test_focus() {
        var control = stackView.createObject(testCase, {initialItem: item, width: 200, height: 200})

        control.forceActiveFocus()
        verify(control.activeFocus)

        control.push({item: textField, immediate: true})
        compare(control.currentItem, textField)
        textField.forceActiveFocus()
        verify(textField.activeFocus)

        control.pop({immediate: true})
        compare(control.currentItem, item)
        verify(control.activeFocus)
        verify(!textField.activeFocus)

        control.destroy()
    }

    function test_find() {
        var control = stackView.createObject(testCase)

        var item1 = component.createObject(control, {objectName: "1"})
        var item2 = component.createObject(control, {objectName: "2"})
        var item3 = component.createObject(control, {objectName: "3"})

        control.push(item1, {immediate: true})
        control.push(item2, {immediate: true})
        control.push(item3, {immediate: true})

        compare(control.find(function(item, index) { return index === 0 }), item1)
        compare(control.find(function(item) { return item.objectName === "1" }), item1)

        compare(control.find(function(item, index) { return index === 1 }), item2)
        compare(control.find(function(item) { return item.objectName === "2" }), item2)

        compare(control.find(function(item, index) { return index === 2 }), item3)
        compare(control.find(function(item) { return item.objectName === "3" }), item3)

        compare(control.find(function() { return false }), null)
        compare(control.find(function() { return true }), item3)

        control.destroy()
    }

    function test_get() {
        var control = stackView.createObject(testCase)

        control.push([item, component, component])

        verify(!control.get(0, true)) // dontLoad=true
        compare(control.get(0, false), item) // dontLoad=false

        verify(!control.get(1, true)) // dontLoad=true
        verify(control.get(1, false)) // dontLoad=false

        verify(control.get(2, true)) // dontLoad=true
        verify(control.get(2, false)) // dontLoad=false

        control.destroy()
    }

    function test_pushpop() {
        var control = stackView.createObject(testCase)

        var item1 = component.createObject(control, {objectName:"1"})
        compare(control.push(item1), item1)
        compare(control.depth, 1)
        compare(control.currentItem, item1)

        var item2 = component.createObject(control, {objectName:"2"})
        compare(control.push({item: item2}), item2)
        compare(control.depth, 2)
        compare(control.currentItem, item2)

        var item3 = component.createObject(control, {objectName:"3"})
        compare(control.push(item3, {}, true), item3)
        compare(control.depth, 3)
        compare(control.currentItem, item3)

        var item4 = component.createObject(control, {objectName:"4"})
        compare(control.push(item4, {}, true, true), item4)
        compare(control.depth, 3)
        compare(control.currentItem, item4)

        var item5 = component.createObject(control, {objectName:"5"})
        compare(control.push({item:item5, immediate:true, replace:true}), item5)
        compare(control.depth, 3)
        compare(control.currentItem, item5)

        var item6 = control.push(component, {objectName:"6"})
        compare(control.depth, 4)
        compare(control.currentItem, item6)

        var item7 = control.push({item:component, properties:{objectName:"7"}})
        compare(control.depth, 5)
        compare(control.currentItem, item7)

        var item8 = component.createObject(control, {objectName:"8"})
        var item9 = component.createObject(control, {objectName:"9"})
        compare(control.push([component, {item:component, properties:{objectName:"?"}}, {item:item8}, item9]), item9)
        compare(control.depth, 9)
        compare(control.currentItem, item9)

        compare(control.pop(), item9)
        compare(control.depth, 8)
        compare(control.currentItem, item8)

        compare(control.pop(), item8)
        compare(control.depth, 7)

        verify(control.pop({immediate:true}))
        verify(control.pop({immediate:false}))
        compare(control.depth, 5)
        compare(control.currentItem, item7)

        compare(control.pop(item5), item7)
        compare(control.depth, 3)
        compare(control.currentItem, item5)

        control.pop(null)
        compare(control.depth, 1)
        compare(control.currentItem, item1)

        control.destroy()
    }
}
