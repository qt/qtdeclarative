// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtTest
import QtQuick.Controls

TestCase {
    id: testCase
    width: 400
    height: 400
    visible: true
    when: windowShown
    name: "BusyIndicator"

    Component {
        id: busyIndicator
        BusyIndicator { }
    }

    Component {
        id: mouseArea
        MouseArea { }
    }
    
    Component {
        id: busyIndicatorInItem
        Item {
            BusyIndicator { }
        }
    }
    
    function init() {
        failOnWarning(/.?/)
    }

    function test_defaults() {
        let control = createTemporaryObject(busyIndicator, testCase)
        verify(control)
        compare(control.running, true)
    }

    function test_running() {
        let control = createTemporaryObject(busyIndicator, testCase)
        verify(control)

        compare(control.running, true)
        control.running = false
        compare(control.running, false)
    }

    // QTBUG-61785
    function test_mouseArea() {
        let ma = createTemporaryObject(mouseArea, testCase, {width: testCase.width, height: testCase.height})
        verify(ma)

        let control = busyIndicator.createObject(ma, {width: testCase.width, height: testCase.height})
        verify(control)

        mousePress(control)
        verify(ma.pressed)

        mouseRelease(control)
        verify(!ma.pressed)

        let touch = touchEvent(control)
        touch.press(0, control).commit()
        verify(ma.pressed)

        touch.release(0, control).commit()
        verify(!ma.pressed)
    }

    // QTBUG-108808
    function test_visibility() {
        let control = createTemporaryObject(busyIndicatorInItem, testCase, {visible: false})
        verify(control)

        let invisibleImage = grabImage(control)
        control.visible = true
        let visibleImage = grabImage(control)

        verify(!invisibleImage.equals(visibleImage))
    }
}
