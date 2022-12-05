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
    name: "ScrollIndicator"

    Component {
        id: scrollIndicator
        ScrollIndicator { }
    }

    Component {
        id: mouseArea
        MouseArea { }
    }

    Component {
        id: flickable
        Flickable {
            width: 100
            height: 100
            contentWidth: 200
            contentHeight: 200
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.HorizontalAndVerticalFlick
        }
    }

    function test_defaults() {
        failOnWarning(/.?/)

        let control = createTemporaryObject(scrollIndicator, testCase)
        verify(control)
    }

    function test_attach() {
        var container = createTemporaryObject(flickable, testCase)
        verify(container)
        waitForRendering(container)

        var vertical = createTemporaryObject(scrollIndicator, null)
        verify(!vertical.parent)
        compare(vertical.size, 0.0)
        compare(vertical.position, 0.0)
        compare(vertical.active, false)
        compare(vertical.orientation, Qt.Vertical)
        compare(vertical.x, 0)
        compare(vertical.y, 0)
        verify(vertical.width > 0)
        verify(vertical.height > 0)

        container.ScrollIndicator.vertical = vertical
        compare(vertical.parent, container)
        compare(vertical.orientation, Qt.Vertical)
        compare(vertical.size, container.visibleArea.heightRatio)
        compare(vertical.position, container.visibleArea.yPosition)
        compare(vertical.x, container.width - vertical.width)
        compare(vertical.y, 0)
        verify(vertical.width > 0)
        compare(vertical.height, container.height)
        // vertical scroll indicator follows flickable's width
        container.width += 10
        compare(vertical.x, container.width - vertical.width)
        vertical.implicitWidth -= 1
        compare(vertical.x, container.width - vertical.width)
        // ...unless explicitly positioned
        vertical.x = 123
        container.width += 10
        compare(vertical.x, 123)

        var horizontal = createTemporaryObject(scrollIndicator, null)
        verify(!horizontal.parent)
        compare(horizontal.size, 0.0)
        compare(horizontal.position, 0.0)
        compare(horizontal.active, false)
        compare(horizontal.orientation, Qt.Vertical)
        compare(horizontal.x, 0)
        compare(horizontal.y, 0)
        verify(horizontal.width > 0)
        verify(horizontal.height > 0)

        container.ScrollIndicator.horizontal = horizontal
        compare(horizontal.parent, container)
        compare(horizontal.orientation, Qt.Horizontal)
        compare(horizontal.size, container.visibleArea.widthRatio)
        compare(horizontal.position, container.visibleArea.xPosition)
        compare(horizontal.x, 0)
        compare(horizontal.y, container.height - horizontal.height)
        compare(horizontal.width, container.width)
        verify(horizontal.height > 0)
        // horizontal scroll indicator follows flickable's height
        container.height += 10
        compare(horizontal.y, container.height - horizontal.height)
        horizontal.implicitHeight -= 1
        compare(horizontal.y, container.height - horizontal.height)
        // ...unless explicitly positioned
        horizontal.y = 123
        container.height += 10
        compare(horizontal.y, 123)

        var velocity = container.maximumFlickVelocity

        compare(container.flicking, false)
        container.flick(-velocity, -velocity)
        compare(container.flicking, true)
        tryCompare(container, "flicking", false)

        compare(vertical.size, container.visibleArea.heightRatio)
        compare(vertical.position, container.visibleArea.yPosition)
        compare(horizontal.size, container.visibleArea.widthRatio)
        compare(horizontal.position, container.visibleArea.xPosition)

        compare(container.flicking, false)
        container.flick(velocity, velocity)
        compare(container.flicking, true)
        tryCompare(container, "flicking", false)

        compare(vertical.size, container.visibleArea.heightRatio)
        compare(vertical.position, container.visibleArea.yPosition)
        compare(horizontal.size, container.visibleArea.widthRatio)
        compare(horizontal.position, container.visibleArea.xPosition)

        var oldY = vertical.y
        var oldHeight = vertical.height
        vertical.parent = testCase
        vertical.y -= 10
        container.height += 10
        compare(vertical.y, oldY - 10)
        compare(vertical.height, oldHeight)

        var oldX = horizontal.x
        var oldWidth = horizontal.width
        horizontal.parent = testCase
        horizontal.x -= 10
        container.width += 10
        compare(horizontal.x, oldX - 10)
        compare(horizontal.width, oldWidth)
    }

    function test_warning() {
        ignoreWarning(/QML TestCase: ScrollIndicator must be attached to a Flickable/)
        testCase.ScrollIndicator.vertical = null
    }

    function test_overshoot() {
        var container = createTemporaryObject(flickable, testCase)
        verify(container)
        waitForRendering(container)

        var vertical = scrollIndicator.createObject(container, {size: 0.5})
        container.ScrollIndicator.vertical = vertical

        var horizontal = scrollIndicator.createObject(container, {size: 0.5})
        container.ScrollIndicator.horizontal = horizontal

        // negative vertical overshoot (pos < 0)
        vertical.position = -0.1
        compare(vertical.contentItem.y, vertical.topPadding)
        compare(vertical.contentItem.height, 0.4 * vertical.availableHeight)

        // positive vertical overshoot (pos + size > 1)
        vertical.position = 0.8
        compare(vertical.contentItem.y, vertical.topPadding + 0.8 * vertical.availableHeight)
        compare(vertical.contentItem.height, 0.2 * vertical.availableHeight)

        // negative horizontal overshoot (pos < 0)
        horizontal.position = -0.1
        compare(horizontal.contentItem.x, horizontal.leftPadding)
        compare(horizontal.contentItem.width, 0.4 * horizontal.availableWidth)

        // positive horizontal overshoot (pos + size > 1)
        horizontal.position = 0.8
        compare(horizontal.contentItem.x, horizontal.leftPadding + 0.8 * horizontal.availableWidth)
        compare(horizontal.contentItem.width, 0.2 * horizontal.availableWidth)
    }

    function test_orientation() {
        var control = createTemporaryObject(scrollIndicator, testCase)
        verify(control)

        compare(control.orientation, Qt.Vertical)
        compare(control.horizontal, false)
        compare(control.vertical, true)

        control.orientation = Qt.Horizontal
        compare(control.orientation, Qt.Horizontal)
        compare(control.horizontal, true)
        compare(control.vertical, false)
    }

    // QTBUG-61785
    function test_mouseArea() {
        var ma = createTemporaryObject(mouseArea, testCase, {width: testCase.width, height: testCase.height})
        verify(ma)

        var control = scrollIndicator.createObject(ma, {active: true, size: 0.9, width: testCase.width, height: testCase.height})
        verify(control)

        mousePress(control)
        verify(ma.pressed)

        mouseRelease(control)
        verify(!ma.pressed)

        var touch = touchEvent(control)
        touch.press(0, control).commit()
        verify(ma.pressed)

        touch.release(0, control).commit()
        verify(!ma.pressed)
    }

    function test_minimumSize() {
        var container = createTemporaryObject(flickable, testCase)
        verify(container)
        waitForRendering(container)

        var vertical = scrollIndicator.createObject(container, {minimumSize: 0.1})
        container.ScrollIndicator.vertical = vertical

        compare(container.visibleArea.heightRatio, 0.5)
        compare(vertical.size, 0.5)
        compare(vertical.visualSize, 0.5)
        compare(vertical.contentItem.height, 0.5 * vertical.availableHeight)

        container.contentHeight = 2000

        compare(container.visibleArea.heightRatio, 0.05)
        compare(vertical.size, 0.05)
        compare(vertical.visualSize, 0.1)
        compare(vertical.contentItem.height, 0.1 * vertical.availableHeight)

        verify(container.atYBeginning)
        compare(container.visibleArea.yPosition, 0.0)
        compare(vertical.position, 0.0)
        compare(vertical.visualPosition, 0.0)
        compare(vertical.contentItem.y, vertical.topPadding)

        container.contentY = 1900

        verify(container.atYEnd)
        compare(container.visibleArea.yPosition, 0.95)
        compare(vertical.position, 0.95)
        compare(vertical.visualPosition, 0.9)
        compare(vertical.contentItem.y, vertical.topPadding + 0.9 * vertical.availableHeight)

        container.contentHeight = 125

        compare(container.visibleArea.heightRatio, 0.8)
        compare(vertical.size, 0.8)
        compare(vertical.visualSize, 0.8)
        compare(vertical.contentItem.height, 0.8 * vertical.availableHeight)

        verify(container.atYEnd)
        compare(container.visibleArea.yPosition, 0.2)
        compare(vertical.position, 0.2)
        compare(vertical.visualPosition, 0.2)
        compare(vertical.contentItem.y, vertical.topPadding + 0.2 * vertical.availableHeight)
    }

    function test_resize() {
        var vertical = createTemporaryObject(scrollIndicator, testCase, { height:200, orientation: Qt.Vertical, size: 0.5, position: 0.5 })
        verify(vertical)

        vertical.size = 0.8
        compare(vertical.position, 0.2)
        compare(vertical.visualPosition, 0.2)
        vertical.size = 0.5
        compare(vertical.position, 0.2)
        compare(vertical.visualPosition, 0.2)


        var horizontal = createTemporaryObject(scrollIndicator, testCase, { width:200, orientation: Qt.Horizontal, size: 0.5, position: 0.5 })
        verify(horizontal)

        horizontal.size = 0.8
        compare(horizontal.position, 0.2)
        compare(horizontal.visualPosition, 0.2)
        horizontal.size = 0.5
        compare(horizontal.position, 0.2)
        compare(horizontal.visualPosition, 0.2)
    }
}
