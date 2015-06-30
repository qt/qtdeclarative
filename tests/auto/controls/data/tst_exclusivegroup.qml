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
    name: "ExclusiveGroup"

    SignalSpy {
        id: currentSpy
        signalName: "currentChanged"
    }

    Component {
        id: exclusiveGroup
        ExclusiveGroup { }
    }

    function init() {
        verify(!currentSpy.target)
        compare(currentSpy.count, 0)
    }

    function cleanup() {
        currentSpy.target = null
        currentSpy.clear()
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
        id: toggleButtons
        Item {
            property ExclusiveGroup group: ExclusiveGroup { id: group }
            property ToggleButton control1: ToggleButton { ExclusiveGroup.group: group }
            property ToggleButton control2: ToggleButton { ExclusiveGroup.group: group }
            property ToggleButton control3: ToggleButton { ExclusiveGroup.group: group }
        }
    }

    function test_controls_data() {
        return [
            { tag: "CheckBox", component: checkBoxes },
            { tag: "RadioButton", component: radioButtons },
            { tag: "Switch", component: switches },
            { tag: "ToggleButton", component: toggleButtons }
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
