/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
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
