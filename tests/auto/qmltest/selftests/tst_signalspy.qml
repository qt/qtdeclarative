// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest

TestCase {
    id: testCase
    name: "SignalSpy"

    Component {
        id: itemWithAmbigiousSignalNameOnChild
        Item {
            property Item child: Item {
                property bool opened: true
                signal opened()
            }
        }
    }

    Component {
        id: signalSpy
        SignalSpy {}
    }

    function init() {
        failOnWarning(/.?/)
    }

    function test_ambigiousSignalName() {
        let control = createTemporaryObject(itemWithAmbigiousSignalNameOnChild, testCase)
        verify(control)

        let openedSpy = signalSpy.createObject(control, {target: control.child, signalName: "opened"})
        verify(openedSpy)
        compare(openedSpy.target, control.child)
        compare(openedSpy.signalName, "opened")
        verify(openedSpy.valid)

        // reset the target, i.e. disconnect the spy
        openedSpy.target = null
        compare(openedSpy.target, null)
        compare(openedSpy.signalName, "opened")
        verify(!openedSpy.valid)

        // connect again to check that it will be disconnected
        // on SignalSpy destruction with no issues/warnings/etc.
        openedSpy.target = control.child
        compare(openedSpy.target, control.child)
        compare(openedSpy.signalName, "opened")
        verify(openedSpy.valid)
    }
}
