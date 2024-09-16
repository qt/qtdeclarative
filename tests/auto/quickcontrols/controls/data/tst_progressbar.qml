// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest
import QtQuick.Controls

TestCase {
    id: testCase
    width: 400
    height: 400
    visible: true
    when: windowShown
    name: "ProgressBar"

    Component {
        id: progressBar
        ProgressBar { }
    }

    function init() {
        failOnWarning(/.?/)
    }

    function test_defaults() {
        let control = createTemporaryObject(progressBar, testCase)
        verify(control)
    }

    function test_value() {
        let control = createTemporaryObject(progressBar, testCase)
        verify(control)

        compare(control.value, 0.0)
        control.value = 0.5
        compare(control.value, 0.5)
        control.value = 1.0
        compare(control.value, 1.0)
        control.value = -1.0
        compare(control.value, 0.0)
        control.value = 2.0
        compare(control.value, 1.0)
    }

    function test_range() {
        let control = createTemporaryObject(progressBar, testCase, {from: 0, to: 100, value: 50})
        verify(control)

        compare(control.from, 0)
        compare(control.to, 100)
        compare(control.value, 50)
        compare(control.position, 0.5)

        control.value = 1000
        compare(control.value, 100)
        compare(control.position, 1)

        control.value = -1
        compare(control.value, 0)
        compare(control.position, 0)

        control.from = 25
        compare(control.from, 25)
        compare(control.value, 25)
        compare(control.position, 0)

        control.to = 75
        compare(control.to, 75)
        compare(control.value, 25)
        compare(control.position, 0)

        control.value = 50
        compare(control.value, 50)
        compare(control.position, 0.5)
    }

    function test_inverted() {
        let control = createTemporaryObject(progressBar, testCase, {from: 1.0, to: -1.0})
        verify(control)

        compare(control.from, 1.0)
        compare(control.to, -1.0)
        compare(control.value, 0.0)
        compare(control.position, 0.5)

        control.value = 2.0
        compare(control.value, 1.0)
        compare(control.position, 0.0)

        control.value = -2.0
        compare(control.value, -1.0)
        compare(control.position, 1.0)

        control.value = 0.0
        compare(control.value, 0.0)
        compare(control.position, 0.5)
    }

    function test_position() {
        let control = createTemporaryObject(progressBar, testCase)
        verify(control)

        compare(control.value, 0)
        compare(control.position, 0)

        control.value = 0.25
        compare(control.value, 0.25)
        compare(control.position, 0.25)

        control.value = 0.75
        compare(control.value, 0.75)
        compare(control.position, 0.75)
    }

    function test_visualPosition() {
        let control = createTemporaryObject(progressBar, testCase)
        verify(control)

        compare(control.value, 0)
        compare(control.visualPosition, 0)

        control.value = 0.25
        compare(control.value, 0.25)
        compare(control.visualPosition, 0.25)

        // RTL locale
        control.locale = Qt.locale("ar_EG")
        compare(control.visualPosition, 0.25)

        // RTL locale + LayoutMirroring
        control.LayoutMirroring.enabled = true
        compare(control.visualPosition, 0.75)

        // LTR locale + LayoutMirroring
        control.locale = Qt.locale("en_US")
        compare(control.visualPosition, 0.75)

        // LTR locale
        control.LayoutMirroring.enabled = false
        compare(control.visualPosition, 0.25)

        // LayoutMirroring
        control.LayoutMirroring.enabled = true
        compare(control.visualPosition, 0.75)
    }

    function test_indeterminate() {
        let control = createTemporaryObject(progressBar, testCase)
        verify(control)
        compare(control.indeterminate, false)

        wait(100)
        control.indeterminate = true
        wait(100)
        // Shouldn't crash...
        control.indeterminate = false
    }
}
