// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest

Item {
    id: root

    TestActivityCommunicator {
        id: communicator
    }

    property bool boolValue: true
    property int integerValue: 0
    property double doubleValue: 0.0
    property string stringValue: ""

    TestCase {
        id: test_getters
        name: "getters"

        function test_bool() {
            root.boolValue = true
            let fetchedValue = communicator.getBoolProperty("boolValue");
            verify(root.boolValue === fetchedValue)
        }

        function test_int() {
            root.integerValue = 42
            let fetchedValue = communicator.getIntProperty("integerValue");
            verify(root.integerValue === fetchedValue)
        }

        function test_double() {
            root.doubleValue = 42.424242
            let fetchedValue = communicator.getDoubleProperty("doubleValue");
            verify(root.doubleValue === fetchedValue)
        }

        function test_string() {
            root.stringValue = "The quick brown fox jumps over the lazy dog"
            let fetchedValue = communicator.getStringProperty("stringValue");
            verify(root.stringValue === fetchedValue)
        }
    }

    TestCase {
        id: test_setters
        name: "setters"

        function test_bool() {
            const newVal = !root.boolValue
            communicator.setBoolProperty("boolValue", newVal);
            verify(root.boolValue === newVal)
        }

        function test_int() {
            const newVal = (42 - root.integerValue)
            communicator.setIntProperty("integerValue", newVal)
            verify(root.integerValue === newVal)
        }

        function test_double() {
            const newVal = (123.123 - root.doubleValue)
            communicator.setDoubleProperty("doubleValue", newVal)
            verify(root.doubleValue === newVal)
        }

        function test_string() {
            const newVal = "The End"
            communicator.setStringProperty("stringValue", newVal);
            verify(root.stringValue === newVal)
        }
    }

    TestCase {
        id: test_root_object_surface
        name: "rootObjectSurface"
        when: windowShown

        readonly property int settleTime: 300

        function test_generatedPropertyChangeListenerFires() {
            communicator.connectGeneratedIntegerValueListener()
            root.integerValue = root.integerValue + 7
            wait(settleTime)
            compare(communicator.listenerCalls(), 1)
        }

        function test_signalNamedWithoutArgumentListFires() {
            communicator.connectRawListener("stringValueChanged")
            root.stringValue = root.stringValue + "x"
            wait(settleTime)
            compare(communicator.listenerCalls(), 1)
        }

        // The generated wrappers cover inherited members too, so the whole
        // root object stays reachable.
        function test_inheritedPropertyIsReachable() {
            verify(communicator.hasProperty("width"))
            verify(communicator.hasProperty("objectName"))

            communicator.setRawStringProperty("objectName", "reached")
            wait(settleTime)
            compare(root.objectName, "reached")
        }
    }
}
