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
    name: "ToolSeparator"

    Component {
        id: toolSeparator
        ToolSeparator {}
    }

    function init() {
        failOnWarning(/.?/)
    }

    function test_defaults() {
        let control = createTemporaryObject(toolSeparator, testCase)
        verify(control)
    }

    function test_size() {
        let control = createTemporaryObject(toolSeparator, testCase);
        verify(control);
        verify(control.width > 1);
        verify(control.height > 1);
    }

    Component {
        id: signalSpyComponent
        SignalSpy {}
    }

    function test_orientation() {
        let control = createTemporaryObject(toolSeparator, testCase);
        verify(control);
        compare(control.horizontal, false);
        compare(control.vertical, true);

        let orientationSpy = signalSpyComponent.createObject(control, { target: control, signalName: "orientationChanged" });

        let originalWidth = control.width;
        let originalHeight = control.height;
        control.orientation = Qt.Horizontal;
        compare(control.orientation, Qt.Horizontal);
        compare(control.width, originalHeight);
        compare(control.height, originalWidth);
        compare(control.horizontal, true);
        compare(control.vertical, false);
        compare(orientationSpy.count, 1);

        control.orientation = Qt.Vertical;
        compare(control.orientation, Qt.Vertical);
        compare(control.width, originalWidth);
        compare(control.height, originalHeight);
        compare(control.horizontal, false);
        compare(control.vertical, true);
        compare(orientationSpy.count, 2);
    }
}
