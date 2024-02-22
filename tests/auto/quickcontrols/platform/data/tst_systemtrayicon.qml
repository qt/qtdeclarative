// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest
import Qt.labs.platform

TestCase {
    id: testCase
    width: 200
    height: 200
    visible: true
    when: windowShown
    name: "SystemTrayIcon"

    Component {
        id: systemTrayIconComponent
        // Check that icon.name can be used in this Qt.labs.platform version
        SystemTrayIcon {
            icon.name: ""
        }
    }

    Component {
        id: signalSpyComponent
        SignalSpy {}
    }

    function test_properties_data() {
        return [
            {tag: "visible", signal: "visibleChanged", init: false, value: true},
            {tag: "icon.source", signal: "iconChanged", init: "", value: "qrc:/tray.png"},
            {tag: "icon.name", signal: "iconChanged", init: "", value: "icon-name"},
            {tag: "tooltip", signal: "tooltipChanged", init: "", value: "tooltip"},
        ]
    }

    function test_properties(data) {
        let icon = createTemporaryObject(systemTrayIconComponent, testCase)
        verify(icon)

        let groupedProperty = data.tag.indexOf(".") !== -1
        let spy = createTemporaryObject(signalSpyComponent, testCase, {
            target: icon, signalName: data.signal
        })
        verify(spy)
        verify(spy.valid)

        let propertyName = groupedProperty ? data.tag.split('.')[1] : data.tag
        let object = !groupedProperty ? icon : icon.icon
        compare(object[propertyName], data.init)

        object[propertyName] = data.value
        compare(spy.count, 1)
        compare(object[propertyName], data.value)

        object[propertyName] = data.value
        compare(spy.count, 1)
    }

    function test_messageIcon() {
        // Q_ENUMS(QPlatformSystemTrayIcon::ActivationReason)
        compare(SystemTrayIcon.NoIcon, 0)
        compare(SystemTrayIcon.Information, 1)
        compare(SystemTrayIcon.Warning, 2)
        compare(SystemTrayIcon.Critical, 3)
    }

    function test_activationReason() {
        // Q_ENUMS(QPlatformSystemTrayIcon::ActivationReason)
        compare(SystemTrayIcon.Unknown, 0)
        compare(SystemTrayIcon.Context, 1)
        compare(SystemTrayIcon.DoubleClick, 2)
        compare(SystemTrayIcon.Trigger, 3)
        compare(SystemTrayIcon.MiddleClick, 4)
    }

    function test_activated() {
        let icon = createTemporaryObject(systemTrayIconComponent, testCase)
        verify(icon)

        let spy = createTemporaryObject(signalSpyComponent, testCase, {
            target: icon, signalName: "activated"
        })
        verify(spy)
        verify(spy.valid)

        icon.activated(SystemTrayIcon.Trigger)
        compare(spy.count, 1)
        compare(spy.signalArguments[0][0], SystemTrayIcon.Trigger)
    }
}
