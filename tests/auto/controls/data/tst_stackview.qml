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

    Item { id: item }
    TextField { id: textField }
    Component { id: component; Item { } }

    Component {
        id: stackView
        StackView { }
    }

    function test_initialItem() {
        var control1 = stackView.createObject(testCase)
        verify(control1)
        compare(control1.currentItem, null)
        control1.destroy()

        var control2 = stackView.createObject(testCase, {initialItem: item})
        verify(control2)
        compare(control2.currentItem, item)
        control2.destroy()

        var control3 = stackView.createObject(testCase, {initialItem: component})
        verify(control3)
        verify(control3.currentItem)
        control3.destroy()
    }

    function test_currentItem() {
        var control = stackView.createObject(testCase, {initialItem: item})
        compare(control.currentItem, item)
        control.push(component)
        verify(control.currentItem !== item)
        control.pop(AbstractStackView.Immediate)
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
        compare(item1.AbstractStackView.status, AbstractStackView.Inactive)
        control.push(item1)
        compare(item1.AbstractStackView.status, AbstractStackView.Active)

        var item2 = component.createObject(control)
        compare(item2.AbstractStackView.status, AbstractStackView.Inactive)
        control.push(item2)
        compare(item2.AbstractStackView.status, AbstractStackView.Activating)
        compare(item1.AbstractStackView.status, AbstractStackView.Deactivating)
        tryCompare(item2.AbstractStackView, "status", AbstractStackView.Active)
        tryCompare(item1.AbstractStackView, "status", AbstractStackView.Inactive)

        control.pop()
        compare(item2.AbstractStackView.status, AbstractStackView.Deactivating)
        compare(item1.AbstractStackView.status, AbstractStackView.Activating)
        tryCompare(item2.AbstractStackView, "status", AbstractStackView.Inactive)
        tryCompare(item1.AbstractStackView, "status", AbstractStackView.Active)

        control.destroy()
    }

    function test_index() {
        var control = stackView.createObject(testCase)

        var item1 = component.createObject(control)
        compare(item1.AbstractStackView.index, -1)
        control.push(item1, AbstractStackView.Immediate)
        compare(item1.AbstractStackView.index, 0)

        var item2 = component.createObject(control)
        compare(item2.AbstractStackView.index, -1)
        control.push(item2, AbstractStackView.Immediate)
        compare(item2.AbstractStackView.index, 1)
        compare(item1.AbstractStackView.index, 0)

        control.pop(AbstractStackView.Immediate)
        compare(item2.AbstractStackView.index, -1)
        compare(item1.AbstractStackView.index, 0)

        control.destroy()
    }

    function test_view() {
        var control = stackView.createObject(testCase)

        var item1 = component.createObject(control)
        compare(item1.AbstractStackView.view, null)
        control.push(item1, AbstractStackView.Immediate)
        compare(item1.AbstractStackView.view, control)

        var item2 = component.createObject(control)
        compare(item2.AbstractStackView.view, null)
        control.push(item2, AbstractStackView.Immediate)
        compare(item2.AbstractStackView.view, control)
        compare(item1.AbstractStackView.view, control)

        control.pop(AbstractStackView.Immediate)
        compare(item2.AbstractStackView.view, null)
        compare(item1.AbstractStackView.view, control)

        control.destroy()
    }

    function test_depth() {
        var control = stackView.createObject(testCase)
        compare(control.depth, 0)
        control.push(item, AbstractStackView.Immediate)
        compare(control.depth, 1)
        control.push(item, AbstractStackView.Immediate)
        compare(control.depth, 2)
        control.pop(AbstractStackView.Immediate)
        compare(control.depth, 1)
        control.push(component, AbstractStackView.Immediate)
        compare(control.depth, 2)
        control.pop(AbstractStackView.Immediate)
        compare(control.depth, 1)
        control.pop(AbstractStackView.Immediate) // ignored
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

        control.push(textField, AbstractStackView.Immediate)
        compare(control.currentItem, textField)
        textField.forceActiveFocus()
        verify(textField.activeFocus)

        control.pop(AbstractStackView.Immediate)
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

        control.push(item1, AbstractStackView.Immediate)
        control.push(item2, AbstractStackView.Immediate)
        control.push(item3, AbstractStackView.Immediate)

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

        control.push([item, component, component], AbstractStackView.Immediate)

        verify(control.get(0, AbstractStackView.DontLoad))
        compare(control.get(0, AbstractStackView.ForceLoad), item)

        verify(!control.get(1, AbstractStackView.DontLoad))

        verify(control.get(2, AbstractStackView.DontLoad))
        verify(control.get(2, AbstractStackView.ForceLoad))

        control.destroy()
    }

    function test_push() {
        var control = stackView.createObject(testCase)

        // missing arguments ### TODO: TestCase.ignoreWarning()
        compare(control.push(), null)

        // nothing to push ### TODO: TestCase.ignoreWarning()
        compare(control.push(AbstractStackView.Immediate), null)

        // push(item)
        var item1 = component.createObject(control, {objectName:"1"})
        compare(control.push(item1, AbstractStackView.Immediate), item1)
        compare(control.depth, 1)
        compare(control.currentItem, item1)

        // push([item])
        var item2 = component.createObject(control, {objectName:"2"})
        compare(control.push([item2], AbstractStackView.Immediate), item2)
        compare(control.depth, 2)
        compare(control.currentItem, item2)

        // push(item, {properties})
        var item3 = component.createObject(control)
        compare(control.push(item3, {objectName:"3"}, AbstractStackView.Immediate), item3)
        compare(item3.objectName, "3")
        compare(control.depth, 3)
        compare(control.currentItem, item3)

        // push([item, {properties}])
        var item4 = component.createObject(control)
        compare(control.push([item4, {objectName:"4"}], AbstractStackView.Immediate), item4)
        compare(item4.objectName, "4")
        compare(control.depth, 4)
        compare(control.currentItem, item4)

        // push(component, {properties})
        var item5 = control.push(component, {objectName:"5"}, AbstractStackView.Immediate)
        compare(item5.objectName, "5")
        compare(control.depth, 5)
        compare(control.currentItem, item5)

        // push([component, {properties}])
        var item6 = control.push([component, {objectName:"6"}], AbstractStackView.Immediate)
        compare(item6.objectName, "6")
        compare(control.depth, 6)
        compare(control.currentItem, item6)

        control.destroy()
    }

    function test_pop() {
        var control = stackView.createObject(testCase)

        var items = []
        for (var i = 0; i < 7; ++i)
            items.push(component.createObject(control, {objectName:i}))

        control.push(items, AbstractStackView.Immediate)

        // too many arguments ### TODO: TestCase.ignoreWarning()
        compare(control.pop(1, 2, 3), null)

        // pop the top most item
        compare(control.pop(AbstractStackView.Immediate), items[6])
        compare(control.depth, 6)
        compare(control.currentItem, items[5])

        // pop down to (but not including) the Nth item
        compare(control.pop(items[3], AbstractStackView.Immediate), items[5])
        compare(control.depth, 4)
        compare(control.currentItem, items[3])

        // pop the top most item
        compare(control.pop(undefined, AbstractStackView.Immediate), items[3])
        compare(control.depth, 3)
        compare(control.currentItem, items[2])

        // don't pop non-existent item
        compare(control.pop(testCase, AbstractStackView.Immediate), null)
        compare(control.depth, 3)
        compare(control.currentItem, items[2])

        // pop all items down to (but not including) the 1st item
        control.pop(null, AbstractStackView.Immediate)
        compare(control.depth, 1)
        compare(control.currentItem, items[0])

        control.destroy()
    }

    function test_replace() {
        var control = stackView.createObject(testCase)

        // missing arguments ### TODO: TestCase.ignoreWarning()
        compare(control.replace(), null)

        // nothing to push ### TODO: TestCase.ignoreWarning()
        compare(control.replace(AbstractStackView.Immediate), null)

        // replace(item)
        var item1 = component.createObject(control, {objectName:"1"})
        compare(control.replace(item1, AbstractStackView.Immediate), item1)
        compare(control.depth, 1)
        compare(control.currentItem, item1)

        // replace([item])
        var item2 = component.createObject(control, {objectName:"2"})
        compare(control.replace([item2], AbstractStackView.Immediate), item2)
        compare(control.depth, 1)
        compare(control.currentItem, item2)

        // replace(item, {properties})
        var item3 = component.createObject(control)
        compare(control.replace(item3, {objectName:"3"}, AbstractStackView.Immediate), item3)
        compare(item3.objectName, "3")
        compare(control.depth, 1)
        compare(control.currentItem, item3)

        // replace([item, {properties}])
        var item4 = component.createObject(control)
        compare(control.replace([item4, {objectName:"4"}], AbstractStackView.Immediate), item4)
        compare(item4.objectName, "4")
        compare(control.depth, 1)
        compare(control.currentItem, item4)

        // replace(component, {properties})
        var item5 = control.replace(component, {objectName:"5"}, AbstractStackView.Immediate)
        compare(item5.objectName, "5")
        compare(control.depth, 1)
        compare(control.currentItem, item5)

        // replace([component, {properties}])
        var item6 = control.replace([component, {objectName:"6"}], AbstractStackView.Immediate)
        compare(item6.objectName, "6")
        compare(control.depth, 1)
        compare(control.currentItem, item6)

        control.destroy()
    }
}
