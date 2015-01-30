/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
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

    function test_defaults() {
        var group = exclusiveGroup.createObject(testCase)
        verify(group)
        verify(!group.current)
        group.destroy()
    }

    Component {
        id: checkable
        QtObject { property bool checked }
    }

    function test_current() {
        var group = exclusiveGroup.createObject(testCase)

        currentSpy.target = group
        verify(currentSpy.valid)

        var checkable1 = checkable.createObject(testCase, {checked: true})
        var checkable2 = checkable.createObject(testCase, {checked: false})
        var checkable3 = checkable.createObject(testCase, {checked: true})

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
            property CheckBox control1: CheckBox { Exclusive.group: group }
            property CheckBox control2: CheckBox { Exclusive.group: group }
            property CheckBox control3: CheckBox { Exclusive.group: group }
        }
    }

    Component {
        id: radioButtons
        Item {
            property ExclusiveGroup group: ExclusiveGroup { id: group }
            property RadioButton control1: RadioButton { Exclusive.group: group }
            property RadioButton control2: RadioButton { Exclusive.group: group }
            property RadioButton control3: RadioButton { Exclusive.group: group }
        }
    }

    Component {
        id: switches
        Item {
            property ExclusiveGroup group: ExclusiveGroup { id: group }
            property Switch control1: Switch { Exclusive.group: group }
            property Switch control2: Switch { Exclusive.group: group }
            property Switch control3: Switch { Exclusive.group: group }
        }
    }

    Component {
        id: toggleButtons
        Item {
            property ExclusiveGroup group: ExclusiveGroup { id: group }
            property ToggleButton control1: ToggleButton { Exclusive.group: group }
            property ToggleButton control2: ToggleButton { Exclusive.group: group }
            property ToggleButton control3: ToggleButton { Exclusive.group: group }
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
