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
    width: 200
    height: 200
    visible: true
    when: windowShown
    name: "ExclusiveGroup"

    Component {
        id: exclusiveGroup
        ExclusiveGroup { }
    }

    Component {
        id: checkableGroup
        ExclusiveGroup {
            QtObject { objectName: "non-checkable" }
            QtObject { objectName: "checkable1"; property bool checked: false }
            QtObject { objectName: "checkable2"; property bool checked: true }
            QtObject { objectName: "checkable3"; property bool checked: false }
        }
    }

    SignalSpy {
        id: currentSpy
        signalName: "currentChanged"
    }

    SignalSpy {
        id: checkablesSpy
        signalName: "checkablesChanged"
    }

    function init() {
        verify(!currentSpy.target)
        compare(currentSpy.count, 0)

        verify(!checkablesSpy.target)
        compare(checkablesSpy.count, 0)
    }

    function cleanup() {
        currentSpy.target = null
        currentSpy.clear()

        checkablesSpy.target = null
        checkablesSpy.clear()
    }

    function test_null() {
        var group = exclusiveGroup.createObject(testCase)
        verify(group)

        group.addCheckable(null)
        group.removeCheckable(null)

        group.destroy()
    }

    Component {
        id: checkable
        QtObject { property bool checked }
    }

    function test_current() {
        var group = exclusiveGroup.createObject(testCase)
        verify(group)

        currentSpy.target = group
        verify(currentSpy.valid)
        verify(!group.current)

        var checkable1 = checkable.createObject(testCase, {checked: true})
        var checkable2 = checkable.createObject(testCase, {checked: false})
        var checkable3 = checkable.createObject(testCase, {checked: true, objectName: "3"})

        // add checked
        group.addCheckable(checkable1)
        compare(group.current, checkable1)
        compare(checkable1.checked, true)
        compare(checkable2.checked, false)
        compare(checkable3.checked, true)
        compare(currentSpy.count, 1)

        // add non-checked
        group.addCheckable(checkable2)
        compare(group.current, checkable1)
        compare(checkable1.checked, true)
        compare(checkable2.checked, false)
        compare(checkable3.checked, true)
        compare(currentSpy.count, 1)

        // add checked
        group.addCheckable(checkable3)
        compare(group.current, checkable3)
        compare(checkable1.checked, false)
        compare(checkable2.checked, false)
        compare(checkable3.checked, true)
        compare(currentSpy.count, 2)

        // change current
        group.current = checkable2
        compare(group.current, checkable2)
        compare(checkable1.checked, false)
        compare(checkable2.checked, true)
        compare(checkable3.checked, false)
        compare(currentSpy.count, 3)

        // check
        checkable1.checked = true
        compare(group.current, checkable1)
        compare(checkable1.checked, true)
        compare(checkable2.checked, false)
        compare(checkable3.checked, false)
        compare(currentSpy.count, 4)

        // remove non-checked
        group.removeCheckable(checkable2)
        compare(group.current, checkable1)
        compare(checkable1.checked, true)
        compare(checkable2.checked, false)
        compare(checkable3.checked, false)
        compare(currentSpy.count, 4)

        // remove checked
        group.removeCheckable(checkable1)
        verify(!group.current)
        compare(checkable1.checked, false)
        compare(checkable2.checked, false)
        compare(checkable3.checked, false)
        compare(currentSpy.count, 5)

        group.destroy()
    }

    function test_checkables() {
        ignoreWarning(Qt.resolvedUrl("tst_exclusivegroup.qml") + ":60:9: QML ExclusiveGroup: The object has no checkedChanged() or toggled() signal.")
        var group = checkableGroup.createObject(testCase)
        verify(group)

        checkablesSpy.target = group
        verify(checkablesSpy.valid)

        compare(group.checkables.length, 3)
        compare(group.checkables[0].objectName, "checkable1")
        compare(group.checkables[1].objectName, "checkable2")
        compare(group.checkables[2].objectName, "checkable3")
        compare(group.current, group.checkables[1])

        var checkable4 = checkable.createObject(testCase, {checked: true})
        var checkable5 = checkable.createObject(testCase, {checked: false})

        group.checkables = [checkable4, checkable5]
        compare(group.checkables.length, 2)
        compare(group.checkables[0], checkable4)
        compare(group.checkables[1], checkable5)
        compare(group.current, checkable4)
        compare(checkablesSpy.count, 3) // clear + 2 * append :/

        var checkable6 = checkable.createObject(testCase, {checked: true})

        group.addCheckable(checkable6)
        compare(group.checkables.length, 3)
        compare(group.checkables[0], checkable4)
        compare(group.checkables[1], checkable5)
        compare(group.checkables[2], checkable6)
        compare(group.current, checkable6)
        compare(checkablesSpy.count, 4)

        group.removeCheckable(checkable4)
        compare(group.checkables.length, 2)
        compare(group.checkables[0], checkable5)
        compare(group.checkables[1], checkable6)
        compare(group.current, checkable6)
        compare(checkablesSpy.count, 5)

        group.checkables = []
        compare(group.checkables.length, 0)
        compare(group.current, null)
        compare(checkablesSpy.count, 6)

        group.destroy()
    }

    Component {
        id: checkBoxes
        Item {
            property ExclusiveGroup group: ExclusiveGroup { id: group }
            property CheckBox control1: CheckBox { ExclusiveGroup.group: group }
            property CheckBox control2: CheckBox { ExclusiveGroup.group: group }
            property CheckBox control3: CheckBox { ExclusiveGroup.group: group }
        }
    }

    Component {
        id: radioButtons
        Item {
            property ExclusiveGroup group: ExclusiveGroup { id: group }
            property RadioButton control1: RadioButton { ExclusiveGroup.group: group }
            property RadioButton control2: RadioButton { ExclusiveGroup.group: group }
            property RadioButton control3: RadioButton { ExclusiveGroup.group: group }
        }
    }

    Component {
        id: switches
        Item {
            property ExclusiveGroup group: ExclusiveGroup { id: group }
            property Switch control1: Switch { ExclusiveGroup.group: group }
            property Switch control2: Switch { ExclusiveGroup.group: group }
            property Switch control3: Switch { ExclusiveGroup.group: group }
        }
    }

    Component {
        id: childControls
        Item {
            id: container
            property ExclusiveGroup group: ExclusiveGroup { id: group; checkables: container.children }
            property alias control1: control1
            property alias control2: control2
            property alias control3: control3
            CheckBox { id: control1 }
            RadioButton { id: control2 }
            Switch { id: control3 }
        }
    }

    function test_controls_data() {
        return [
            { tag: "CheckBox", component: checkBoxes },
            { tag: "RadioButton", component: radioButtons },
            { tag: "Switch", component: switches },
            { tag: "Children", component: childControls }
        ]
    }

    function test_controls(data) {
        var container = data.component.createObject(testCase)
        verify(container)

        verify(!container.group.current)

        container.control1.checked = true
        compare(container.group.current, container.control1)
        compare(container.control1.checked, true)
        compare(container.control2.checked, false)
        compare(container.control3.checked, false)

        container.control2.checked = true
        compare(container.group.current, container.control2)
        compare(container.control1.checked, false)
        compare(container.control2.checked, true)
        compare(container.control3.checked, false)

        container.control3.checked = true
        compare(container.group.current, container.control3)
        compare(container.control1.checked, false)
        compare(container.control2.checked, false)
        compare(container.control3.checked, true)

        container.destroy()
    }
}
