// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtTest 1.1

Button {
    id: button
    onClicked: text = "Clicked"

    SignalSpy {
        id: spy
        target: button
        signalName: "clicked"
    }

    TestCase {
        name: "ButtonClick"
        when: windowShown

        function test_click() {

            compare(spy.count, 0)
            button.clicked(1, 2);
            compare(button.text, "Clicked");

            compare(spy.count, 1)
            compare(spy.signalArguments.length, 1)
            compare(spy.signalArguments[0][0], 1)
            compare(spy.signalArguments[0][1], 2)
            verify(spy.valid)
            spy.clear()
            compare(spy.count, 0)
            verify(spy.valid)
            compare(spy.signalArguments.length, 0)
            spy.signalName = ""
            verify(!spy.valid)
        }
    }
}
