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
    name: "PageIndicator"

    Component {
        id: pageIndicator
        PageIndicator {
            objectName: "pageIndicator"
            contentItem.objectName: "pageIndicatorContentItem"
        }
    }

    Component {
        id: mouseArea
        MouseArea {
            objectName: "mouseArea"
        }
    }

    function test_defaults() {
        failOnWarning(/.?/)

        let control = createTemporaryObject(pageIndicator, testCase)
        verify(control)
    }

    function test_count() {
        var control = createTemporaryObject(pageIndicator, testCase)
        verify(control)

        compare(control.count, 0)
        control.count = 3
        compare(control.count, 3)
    }

    function test_currentIndex() {
        var control = createTemporaryObject(pageIndicator, testCase)
        verify(control)

        compare(control.currentIndex, 0)
        control.currentIndex = 5
        compare(control.currentIndex, 5)
    }

    function test_interactive_data() {
        return [
            { tag: "mouse", touch: false },
            { tag: "touch", touch: true }
        ]
    }

    function test_interactive(data) {
        var control = createTemporaryObject(pageIndicator, testCase, {count: 5, spacing: 10, topPadding: 10, leftPadding: 10, rightPadding: 10, bottomPadding: 10})
        verify(control)

        verify(!control.interactive)
        compare(control.currentIndex, 0)

        var touch = touchEvent(control)

        if (data.touch)
            touch.press(0, control).commit().release(0, control).commit()
        else
            mouseClick(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.currentIndex, 0)

        control.interactive = true
        verify(control.interactive)

        if (data.touch)
            touch.press(0, control).commit().release(0, control).commit()
        else
            mouseClick(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.currentIndex, 2)

        // test also clicking outside delegates => the nearest should be selected
        for (var i = 0; i < control.count; ++i) {
            var child = control.contentItem.children[i]

            var points = [
                Qt.point(child.width / 2, -2), // top
                Qt.point(-2, child.height / 2), // left
                Qt.point(child.width + 2, child.height / 2), // right
                Qt.point(child.width / 2, child.height + 2), // bottom

                Qt.point(-2, -2), // top-left
                Qt.point(child.width + 2, -2), // top-right
                Qt.point(-2, child.height + 2), // bottom-left
                Qt.point(child.width + 2, child.height + 2), // bottom-right
            ]

            for (var j = 0; j < points.length; ++j) {
                control.currentIndex = -1
                compare(control.currentIndex, -1)

                var point = points[j]
                var pos = control.mapFromItem(child, x, y)
                if (data.touch)
                    touch.press(0, control, pos.x, pos.y).commit().release(0, control, pos.x, pos.y).commit()
                else
                    mouseClick(control, pos.x, pos.y, Qt.LeftButton)
                compare(control.currentIndex, i)
            }
        }
    }

    function test_mouseArea_data() {
        return [
            { tag: "interactive", interactive: true },
            { tag: "non-interactive", interactive: false }
        ]
    }

    // QTBUG-61785
    function test_mouseArea(data) {
        var ma = createTemporaryObject(mouseArea, testCase, {width: testCase.width, height: testCase.height})
        verify(ma)

        var control = pageIndicator.createObject(ma, {count: 5, interactive: data.interactive, width: testCase.width, height: testCase.height})
        verify(control)

        compare(control.interactive, data.interactive)

        mousePress(control)
        compare(ma.pressed, !data.interactive)

        mouseRelease(control)
        verify(!ma.pressed)

        var touch = touchEvent(control)
        touch.press(0, control).commit()
        compare(ma.pressed, !data.interactive)

        touch.release(0, control).commit()
        verify(!ma.pressed)
    }
}
