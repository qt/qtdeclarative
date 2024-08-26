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
    name: "ScrollBar"

    Component {
        id: signalSpy
        SignalSpy { }
    }

    Component {
        id: defaultScrollBar

        ScrollBar {}
    }

    Component {
        id: scrollBar
        ScrollBar {
            objectName: "scrollBar"
            padding: 0
            minimumSize: 0
        }
    }

    Component {
        id: scrollBarWithDefaultPadding
        ScrollBar {
            minimumSize: 0
        }
    }

    Component {
        id: flickable
        Flickable {
            objectName: "flickable"
            width: 100
            height: 100
            contentWidth: 200
            contentHeight: 200
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.HorizontalAndVerticalFlick
        }
    }

    function init() {
        failOnWarning(/.?/)
    }

    function test_defaults() {
        let control = createTemporaryObject(defaultScrollBar, testCase)
        verify(control)
    }

    function test_attach() {
        let container = createTemporaryObject(flickable, testCase)
        verify(container)
        waitForRendering(container)

        let vertical = scrollBar.createObject(null, { objectName: "verticalScrollBar" })
        verify(!vertical.parent)
        compare(vertical.size, 0.0)
        compare(vertical.position, 0.0)
        compare(vertical.active, false)
        compare(vertical.orientation, Qt.Vertical)
        compare(vertical.x, 0)
        compare(vertical.y, 0)
        verify(vertical.width > 0)
        verify(vertical.height > 0)

        container.ScrollBar.vertical = vertical
        compare(vertical.parent, container)
        compare(vertical.orientation, Qt.Vertical)
        compare(vertical.size, container.visibleArea.heightRatio)
        compare(vertical.position, container.visibleArea.yPosition)
        compare(vertical.x, container.width - vertical.width)
        compare(vertical.y, 0)
        verify(vertical.width > 0)
        compare(vertical.height, container.height)
        // vertical scroll bar follows flickable's width
        container.width += 10
        compare(vertical.x, container.width - vertical.width)
        vertical.implicitWidth -= 2
        compare(vertical.x, container.width - vertical.width)
        // ...unless explicitly positioned
        vertical.x = 123
        container.width += 10
        compare(vertical.x, 123)

        let horizontal = createTemporaryObject(scrollBar, null, { objectName: "horizontalScrollBar" })
        verify(!horizontal.parent)
        compare(horizontal.size, 0.0)
        compare(horizontal.position, 0.0)
        compare(horizontal.active, false)
        compare(horizontal.orientation, Qt.Vertical)
        compare(horizontal.x, 0)
        compare(horizontal.y, 0)
        verify(horizontal.width > 0)
        verify(horizontal.height > 0)

        container.ScrollBar.horizontal = horizontal
        compare(horizontal.parent, container)
        compare(horizontal.orientation, Qt.Horizontal)
        compare(horizontal.size, container.visibleArea.widthRatio)
        compare(horizontal.position, container.visibleArea.xPosition)
        compare(horizontal.x, 0)
        compare(horizontal.y, container.height - horizontal.height)
        compare(horizontal.width, container.width)
        verify(horizontal.height > 0)
        // horizontal scroll bar follows flickable's height
        container.height += 10
        compare(horizontal.y, container.height - horizontal.height)
        horizontal.implicitHeight -= 2
        compare(horizontal.y, container.height - horizontal.height)
        // ...unless explicitly positioned
        horizontal.y = 123
        container.height += 10
        compare(horizontal.y, 123)

        let velocity = container.maximumFlickVelocity

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

        let oldY = vertical.y
        let oldHeight = vertical.height
        vertical.parent = testCase
        vertical.y -= 10
        container.height += 10
        compare(vertical.y, oldY - 10)
        compare(vertical.height, oldHeight)

        let oldX = horizontal.x
        let oldWidth = horizontal.width
        horizontal.parent = testCase
        horizontal.x -= 10
        container.width += 10
        compare(horizontal.x, oldX - 10)
        compare(horizontal.width, oldWidth)
    }

    function test_attachTwice() {
        let container = createTemporaryObject(flickable, testCase)
        verify(container)
        waitForRendering(container)

        container.ScrollBar.vertical = scrollBar.createObject(container, { objectName: "oldVerticalScrollBar" })
        verify(container.ScrollBar.vertical)
        let oldVerticalScrollBar = findChild(container, "oldVerticalScrollBar")
        verify(oldVerticalScrollBar)
        verify(oldVerticalScrollBar.visible)

        container.ScrollBar.horizontal = scrollBar.createObject(container, { objectName: "oldHorizontalScrollBar" })
        verify(container.ScrollBar.horizontal)
        let oldHorizontalScrollBar = findChild(container, "oldHorizontalScrollBar")
        verify(oldHorizontalScrollBar)
        verify(oldHorizontalScrollBar.visible)

        container.ScrollBar.vertical = scrollBar.createObject(container, { objectName: "newVerticalScrollBar" })
        let newVerticalScrollBar = findChild(container, "newVerticalScrollBar")
        verify(newVerticalScrollBar)
        verify(newVerticalScrollBar.visible)
        verify(!oldVerticalScrollBar.visible)

        container.ScrollBar.horizontal = scrollBar.createObject(container, { objectName: "newHorizontalScrollBar" })
        let newHorizontalScrollBar = findChild(container, "newHorizontalScrollBar")
        verify(newHorizontalScrollBar)
        verify(newHorizontalScrollBar.visible)
        verify(!oldHorizontalScrollBar.visible)
    }

    function getGrooveRange(scrollbar) {
        return {
            start: {    // top left
               x: scrollbar.orientation === Qt.Horizontal ? scrollbar.leftPadding : 0,
               y: scrollbar.orientation === Qt.Vertical ? scrollbar.topPadding : 0
            },
            end: {      // bottom right, (inclusive, last pixel position of the groove)
               x: (scrollbar.orientation === Qt.Horizontal ? scrollbar.width - scrollbar.rightPadding : scrollbar.width) - 1,
               y: (scrollbar.orientation === Qt.Vertical ? scrollbar.height - scrollbar.bottomPadding : scrollbar.height) - 1
            },
            width : scrollbar.width - scrollbar.leftPadding - scrollbar.rightPadding,
            height: scrollbar.height - scrollbar.topPadding - scrollbar.bottomPadding
        }
    }

    function test_mouse_data() {
        return [
            { tag: "horizontal", properties: { visible: true, orientation: Qt.Horizontal, width: testCase.width } },
            { tag: "vertical", properties: { visible: true, orientation: Qt.Vertical, height: testCase.height } }
        ]
    }

    function test_mouse(data) {
        let control = createTemporaryObject(scrollBarWithDefaultPadding, testCase, data.properties)
        verify(control)

        let grooveRange = getGrooveRange(control)

        let pressedSpy = signalSpy.createObject(control, {target: control, signalName: "pressedChanged"})
        verify(pressedSpy.valid)

        mousePress(control, grooveRange.start.x, grooveRange.start.y, Qt.LeftButton)
        compare(pressedSpy.count, 1)
        compare(control.pressed, true)
        compare(control.position, 0.0)

        mouseMove(control, -control.width, -control.height, 0)
        compare(pressedSpy.count, 1)
        compare(control.pressed, true)
        compare(control.position, 0.0)

        mouseMove(control, control.width * 0.5, control.height * 0.5, 0)
        compare(pressedSpy.count, 1)
        compare(control.pressed, true)
        verify(control.position, 0.5)

        mouseRelease(control, control.width * 0.5, control.height * 0.5, Qt.LeftButton)
        compare(pressedSpy.count, 2)
        compare(control.pressed, false)
        compare(control.position, 0.5)

        mousePress(control, grooveRange.end.x, grooveRange.end.y, Qt.LeftButton)
        compare(pressedSpy.count, 3)
        compare(control.pressed, true)
        // We can't click on right and bottom edge, so click to (grooveRange.end),
        // and move mouse to (grooveRange.end.x + 1, grooveRange.end.y + 1)
        mouseMove(control, grooveRange.end.x + 1, grooveRange.end.y + 1, 0)
        compare(control.position, 1.0)

        mouseMove(control, control.width * 2, control.height * 2, 0)
        compare(pressedSpy.count, 3)
        compare(control.pressed, true)
        compare(control.position, 1.0)

        mouseMove(control, grooveRange.start.x + grooveRange.width * 0.75, grooveRange.start.y + grooveRange.height * 0.75, 0)
        compare(pressedSpy.count, 3)
        compare(control.pressed, true)
        fuzzyCompare(control.position, 0.75, 0.01)

        mouseRelease(control, grooveRange.start.x + grooveRange.width * 0.25, grooveRange.start.y + grooveRange.height * 0.25, Qt.LeftButton)
        compare(pressedSpy.count, 4)
        compare(control.pressed, false)
        fuzzyCompare(control.position, 0.25, 0.01)

        if (control.__decreaseVisual.indicator !== null) {
            let p = control.__decreaseVisual.indicator.mapToItem(control, Qt.point(0, 0))
            mousePress(control, p.x, p.y, Qt.LeftButton)
            compare(pressedSpy.count, 4)
            compare(control.pressed, false)
            compare(control.__decreaseVisual.pressed, true)
            fuzzyCompare(control.position, 0.15, 0.01)
            mouseRelease(control.__decreaseVisual.indicator, 0, 0, Qt.LeftButton)
            compare(control.__decreaseVisual.pressed, false)

            p = control.__increaseVisual.indicator.mapToItem(control, Qt.point(0, 0))
            mousePress(control, p.x, p.y, Qt.LeftButton)
            compare(pressedSpy.count, 4)
            compare(control.pressed, false)
            compare(control.__increaseVisual.pressed, true)
            fuzzyCompare(control.position, 0.25, 0.01)
            mouseRelease(control.__increaseVisual.indicator, 0, 0, Qt.LeftButton)
            compare(control.__increaseVisual.pressed, false)
        }
    }

    function test_touch_data() {
        return [
            { tag: "horizontal", properties: { visible: true, orientation: Qt.Horizontal, width: testCase.width } },
            { tag: "vertical", properties: { visible: true, orientation: Qt.Vertical, height: testCase.height } }
        ]
    }

    function test_touch(data) {
        let control = createTemporaryObject(scrollBar, testCase, data.properties)
        verify(control)

        let pressedSpy = signalSpy.createObject(control, {target: control, signalName: "pressedChanged"})
        verify(pressedSpy.valid)

        control.width += (control.leftPadding + control.rightPadding)
        control.height += (control.topPadding + control.bottomPadding)
        let availableSlideWidth = 0
        let availableSlideHeight = 0

        let p0 = {}
        if (control.orientation === Qt.Horizontal) {
            availableSlideWidth = control.width - control.rightPadding - control.leftPadding
            p0 = { x = control.leftPadding, y = control.height/2 }
        } else {
            availableSlideHeight = control.height - control.bottomPadding - control.topPadding
            p0 = { x = control.width/2, y = control.topPadding}
        }

        let touch = touchEvent(control)

        touch.press(0, control, p0.x, p0.y).commit()
        compare(pressedSpy.count, 1)
        compare(control.pressed, true)
        compare(control.position, 0.0)

        touch.move(0, control, -control.width, -control.height).commit()
        compare(pressedSpy.count, 1)
        compare(control.pressed, true)
        compare(control.position, 0.0)

        touch.move(0, control, p0.x + availableSlideWidth * 0.5, p0.y + availableSlideHeight * 0.5).commit()
        compare(pressedSpy.count, 1)
        compare(control.pressed, true)
        verify(control.position, 0.5)

        touch.release(0, control, p0.x + availableSlideWidth * 0.5, p0.y + availableSlideHeight * 0.5).commit()
        compare(pressedSpy.count, 2)
        compare(control.pressed, false)
        compare(control.position, 0.5)

        touch.press(0, control, p0.x + availableSlideWidth - 1, p0.y + availableSlideHeight - 1).commit()
        compare(pressedSpy.count, 3)
        compare(control.pressed, true)
        compare(control.position, 0.5)

        touch.move(0, control, control.width * 2, control.height * 2).commit()
        compare(pressedSpy.count, 3)
        compare(control.pressed, true)
        compare(control.position, 1.0)

        touch.move(0, control, p0.x + availableSlideWidth * 0.75, p0.y + availableSlideHeight * 0.75).commit()
        compare(pressedSpy.count, 3)
        compare(control.pressed, true)
        compare(control.position, 0.75)

        touch.release(0, control, p0.x + availableSlideWidth * 0.25, p0.y + availableSlideHeight * 0.25).commit()
        compare(pressedSpy.count, 4)
        compare(control.pressed, false)
        compare(control.position, 0.25)
    }

    function test_multiTouch() {
        let control1 = createTemporaryObject(scrollBar, testCase)
        verify(control1)

        control1.height = 200 + (control1.topPadding + control1.bottomPadding)

        let grooveRange = getGrooveRange(control1)

        let pressedCount1 = 0
        let movedCount1 = 0

        let pressedSpy1 = signalSpy.createObject(control1, {target: control1, signalName: "pressedChanged"})
        verify(pressedSpy1.valid)

        let positionSpy1 = signalSpy.createObject(control1, {target: control1, signalName: "positionChanged"})
        verify(positionSpy1.valid)

        let touch = touchEvent(control1)
        touch.press(0, control1, grooveRange.start.x, grooveRange.start.y).commit().move(0, control1, grooveRange.end.x+1, grooveRange.end.y+1).commit()

        compare(pressedSpy1.count, ++pressedCount1)
        compare(positionSpy1.count, ++movedCount1)
        compare(control1.pressed, true)
        compare(control1.position, 1.0)

        // second touch point on the same control is ignored
        touch.stationary(0).press(1, control1, grooveRange.start.x, grooveRange.start.y).commit()
        touch.stationary(0).move(1).commit()
        touch.stationary(0).release(1).commit()

        compare(pressedSpy1.count, pressedCount1)
        compare(positionSpy1.count, movedCount1)
        compare(control1.pressed, true)
        compare(control1.position, 1.0)

        let control2 = createTemporaryObject(scrollBar, testCase, {y: control1.height})
        verify(control2)
        control2.height = 200 + (control2.topPadding + control2.bottomPadding)

        let pressedCount2 = 0
        let movedCount2 = 0

        let pressedSpy2 = signalSpy.createObject(control2, {target: control2, signalName: "pressedChanged"})
        verify(pressedSpy2.valid)

        let positionSpy2 = signalSpy.createObject(control2, {target: control2, signalName: "positionChanged"})
        verify(positionSpy2.valid)

        // press the second scrollbar
        touch.stationary(0).press(2, control2, grooveRange.start.x, grooveRange.start.y).commit()

        compare(pressedSpy2.count, ++pressedCount2)
        compare(positionSpy2.count, movedCount2)
        compare(control2.pressed, true)
        compare(control2.position, 0.0)

        compare(pressedSpy1.count, pressedCount1)
        compare(positionSpy1.count, movedCount1)
        compare(control1.pressed, true)
        compare(control1.position, 1.0)

        // move both scrollbars
        touch.move(0, control1).move(2, control2).commit()

        compare(pressedSpy2.count, pressedCount2)
        compare(positionSpy2.count, ++movedCount2)
        compare(control2.pressed, true)
        compare(control2.position, 0.5)

        compare(pressedSpy1.count, pressedCount1)
        compare(positionSpy1.count, ++movedCount1)
        compare(control1.pressed, true)
        compare(control1.position, 0.5)

        // release both scrollbars
        touch.release(0, control1).release(2, control2).commit()

        compare(pressedSpy2.count, ++pressedCount2)
        compare(positionSpy2.count, movedCount2)
        compare(control2.pressed, false)
        compare(control2.position, 0.5)

        compare(pressedSpy1.count, ++pressedCount1)
        compare(positionSpy1.count, movedCount1)
        compare(control1.pressed, false)
        compare(control1.position, 0.5)
    }

    function test_increase_decrease_data() {
        return [
            { tag: "increase:active", increase: true, active: true },
            { tag: "decrease:active", increase: false, active: true },
            { tag: "increase:inactive", increase: true, active: false },
            { tag: "decrease:inactive", increase: false, active: false }
        ]
    }

    function test_increase_decrease(data) {
        let control = createTemporaryObject(scrollBar, testCase, {position: 0.5, active: data.active})
        verify(control)

        if (data.increase) {
            control.increase()
            compare(control.position, 0.6)
        } else {
            control.decrease()
            compare(control.position, 0.4)
        }
        compare(control.active, data.active)
    }

    function test_stepSize_data() {
        return [
            { tag: "0.0", stepSize: 0.0 },
            { tag: "0.1", stepSize: 0.1 },
            { tag: "0.5", stepSize: 0.5 }
        ]
    }

    function test_stepSize(data) {
        let control = createTemporaryObject(scrollBar, testCase, {stepSize: data.stepSize})
        verify(control)

        compare(control.stepSize, data.stepSize)
        compare(control.position, 0.0)

        let count = 10
        if (data.stepSize !== 0.0)
            count = 1.0 / data.stepSize

        // increase until 1.0
        for (let i = 1; i <= count; ++i) {
            control.increase()
            compare(control.position, i / count)
        }
        control.increase()
        compare(control.position, 1.0)

        // decrease until 0.0
        for (let d = count - 1; d >= 0; --d) {
            control.decrease()
            compare(control.position, d / count)
        }
        control.decrease()
        compare(control.position, 0.0)
    }

    function test_padding_data() {
        return [
            { tag: "horizontal", properties: { visible: true, orientation: Qt.Horizontal, width: testCase.width, leftPadding: testCase.width * 0.1 } },
            { tag: "vertical", properties: { visible: true, orientation: Qt.Vertical, height: testCase.height, topPadding: testCase.height * 0.1 } }
        ]
    }

    function test_padding(data) {
        let control = createTemporaryObject(scrollBar, testCase, data.properties)

        mousePress(control, control.leftPadding + control.availableWidth * 0.5, control.topPadding + control.availableHeight * 0.5, Qt.LeftButton)
        mouseRelease(control, control.leftPadding + control.availableWidth * 0.5, control.topPadding + control.availableHeight * 0.5, Qt.LeftButton)

        compare(control.position, 0.5)
    }

    function test_warning() {
        ignoreWarning(/.*QML TestCase: ScrollBar attached property must be attached to an object deriving from Flickable or ScrollView/)
        testCase.ScrollBar.vertical = null
    }

    function test_mirrored() {
        let container = createTemporaryObject(flickable, testCase)
        verify(container)
        waitForRendering(container)

        container.ScrollBar.vertical = scrollBar.createObject(container)
        compare(container.ScrollBar.vertical.x, container.width - container.ScrollBar.vertical.width)
        container.ScrollBar.vertical.locale = Qt.locale("ar_EG")
        compare(container.ScrollBar.vertical.x, container.width - container.ScrollBar.vertical.width)
    }

    function test_hover_data() {
        return [
            { tag: "enabled", hoverEnabled: true, interactive: true },
            { tag: "disabled", hoverEnabled: false, interactive: true },
            { tag: "non-interactive", hoverEnabled: true, interactive: false }
        ]
    }

    function test_hover(data) {
        let control = createTemporaryObject(scrollBar, testCase, {hoverEnabled: data.hoverEnabled, interactive: data.interactive})
        verify(control)

        compare(control.hovered, false)

        mouseMove(control, control.width / 2, control.height / 2)
        compare(control.hovered, data.hoverEnabled)
        compare(control.active, data.hoverEnabled && data.interactive)

        mouseMove(control, -1, -1)
        compare(control.hovered, false)
        compare(control.active, false)
    }

    function test_snapMode_data() {
        return [
            { tag: "NoSnap", snapMode: ScrollBar.NoSnap, stepSize: 0.1, size: 0.2, width: 100, steps: 80 }, /* 0.8*100 */
            { tag: "NoSnap2", snapMode: ScrollBar.NoSnap, stepSize: 0.2, size: 0.1, width: 200, steps: 180 }, /* 0.9*200 */

            { tag: "SnapAlways", snapMode: ScrollBar.SnapAlways, stepSize: 0.1, size: 0.2, width: 100, steps: 10 },
            { tag: "SnapAlways2", snapMode: ScrollBar.SnapAlways, stepSize: 0.2, size: 0.125, width: 200, steps: 5 },

            { tag: "SnapOnRelease", snapMode: ScrollBar.SnapOnRelease, stepSize: 0.1, size: 0.2, width: 100, steps: 80 }, /* 0.8*100 */
            { tag: "SnapOnRelease2", snapMode: ScrollBar.SnapOnRelease, stepSize: 0.2, size: 0.1, width: 200, steps: 180 }, /* 0.9*200 */
        ]
    }

    function test_snapMode_mouse_data() {
        return test_snapMode_data()
    }

    function test_snapMode_mouse(data) {
        let control = createTemporaryObject(scrollBar, testCase, {snapMode: data.snapMode, orientation: Qt.Horizontal, stepSize: data.stepSize, size: data.size, width: data.width})
        verify(control)
        // In case the slider is surrounded with decrease/increase buttons
        // Adjust slider width so that slider area is a whole number (to avoid rounding errors)
        control.width += control.leftPadding + control.rightPadding

        function snappedPosition(pos) {
            let effectiveStep = control.stepSize * (1.0 - control.size)
            return Math.round(pos / effectiveStep) * effectiveStep
        }

        function boundPosition(pos) {
            return Math.max(0, Math.min(pos, 1.0 - control.size))
        }

        let minHandlePos = control.leftPadding
        let maxHandlePos = control.width - control.rightPadding
        let availableSlideWidth = maxHandlePos - minHandlePos
        mousePress(control, minHandlePos, 0)
        compare(control.position, 0)

        mouseMove(control, minHandlePos + availableSlideWidth * 0.3, 0)
        let expectedMovePos = 0.3
        if (control.snapMode === ScrollBar.SnapAlways) {
            expectedMovePos = snappedPosition(expectedMovePos)
            verify(expectedMovePos !== 0.3)
        }
        compare(control.position, expectedMovePos)

        mouseRelease(control, minHandlePos + availableSlideWidth * 0.75, 0)
        let expectedReleasePos = 0.75
        if (control.snapMode !== ScrollBar.NoSnap) {
            expectedReleasePos = snappedPosition(expectedReleasePos)
            verify(expectedReleasePos !== 0.75)
        }
        compare(control.position, expectedReleasePos)

        control.position = 0
        mousePress(control, minHandlePos, 0)

        let steps = 0
        let prevPos = 0

        for (let x = minHandlePos; x < maxHandlePos; ++x) {
            mouseMove(control, x, 0)
            expectedMovePos = boundPosition((x - minHandlePos) / availableSlideWidth)
            if (control.snapMode === ScrollBar.SnapAlways)
                expectedMovePos = snappedPosition(expectedMovePos)
            compare(control.position, expectedMovePos)

            if (control.position !== prevPos)
                ++steps
            prevPos = control.position
        }
        compare(steps, data.steps)

        mouseRelease(control, maxHandlePos - 1, 0)
    }

    function test_snapMode_touch_data() {
        return test_snapMode_data()
    }

    function test_snapMode_touch(data) {
        let control = createTemporaryObject(scrollBar, testCase, {snapMode: data.snapMode, orientation: Qt.Horizontal, stepSize: data.stepSize, size: data.size, width: data.width})
        verify(control)
        // In case the slider is surrounded with decrease/increase buttons
        // Adjust slider width so that slider area is a whole number (to avoid rounding errors)
        control.width += control.leftPadding + control.rightPadding

        function snappedPosition(pos) {
            let effectiveStep = control.stepSize * (1.0 - control.size)
            return Math.round(pos / effectiveStep) * effectiveStep
        }

        function boundPosition(pos) {
            return Math.max(0, Math.min(pos, 1.0 - control.size))
        }

        let touch = touchEvent(control)

        let minHandlePos = control.leftPadding
        let maxHandlePos = control.width - control.rightPadding
        let availableSlideWidth = maxHandlePos - minHandlePos
        touch.press(0, control, minHandlePos, 0).commit()
        compare(control.position, 0)

        touch.move(0, control, minHandlePos + availableSlideWidth*0.3, 0).commit()
        let expectedMovePos = 0.3
        if (control.snapMode === ScrollBar.SnapAlways) {
            expectedMovePos = snappedPosition(expectedMovePos)
            verify(expectedMovePos !== 0.3)
        }
        compare(control.position, expectedMovePos)

        touch.release(0, control, minHandlePos + availableSlideWidth*0.75, 0).commit()
        let expectedReleasePos = 0.75
        if (control.snapMode !== ScrollBar.NoSnap) {
            expectedReleasePos = snappedPosition(expectedReleasePos)
            verify(expectedReleasePos !== 0.75)
        }
        compare(control.position, expectedReleasePos)

        control.position = 0
        touch.press(0, control, minHandlePos, 0).commit()

        let steps = 0
        let prevPos = 0

        for (let x = minHandlePos; x < maxHandlePos; ++x) {
            touch.move(0, control, x, 0).commit()
            expectedMovePos = boundPosition((x - minHandlePos) / availableSlideWidth)
            if (control.snapMode === ScrollBar.SnapAlways)
                expectedMovePos = snappedPosition(expectedMovePos)
            compare(control.position, expectedMovePos)

            if (control.position !== prevPos)
                ++steps
            prevPos = control.position
        }
        compare(steps, data.steps)

        touch.release(0, control, maxHandlePos - 1).commit()
    }

    function test_interactive_data() {
        return [
            { tag: "true", interactive: true },
            { tag: "false", interactive: false }
        ]
    }

    function test_interactive(data) {
        let control = createTemporaryObject(scrollBar, testCase, {interactive: data.interactive})
        verify(control)

        compare(control.interactive, data.interactive)
        // 200 pixels tall to avoid rounding errors further on
        control.height = 200 + (control.topPadding + control.bottomPadding)

        // press-move-release
        mousePress(control, control.width/2, control.topPadding, Qt.LeftButton)
        compare(control.pressed, data.interactive)

        mouseMove(control, control.width / 2, control.height / 2)
        compare(control.position, data.interactive ? 0.5 : 0.0)

        mouseRelease(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, false)

        // change to non-interactive while pressed
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, data.interactive)

        mouseMove(control, control.width, control.height)
        compare(control.position, data.interactive ? 1.0 : 0.0)

        control.interactive = false
        compare(control.interactive, false)
        compare(control.pressed, false)

        mouseMove(control, control.width / 2, control.height / 2)
        compare(control.position, data.interactive ? 1.0 : 0.0)

        mouseRelease(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, false)

        // change back to interactive & try press-move-release again
        control.interactive = true
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, true)

        mouseMove(control, 0, 0)
        compare(control.position, 0.0)

        mouseRelease(control, 0, 0, Qt.LeftButton)
        compare(control.pressed, false)
    }

    function test_policy() {
        let control = createTemporaryObject(scrollBar, testCase, {active: true})
        verify(control)

        compare(ScrollBar.AsNeeded, Qt.ScrollBarAsNeeded)
        compare(ScrollBar.AlwaysOff, Qt.ScrollBarAlwaysOff)
        compare(ScrollBar.AlwaysOn, Qt.ScrollBarAlwaysOn)

        compare(control.visible, true)
        compare(control.policy, ScrollBar.AsNeeded)

        // some ScrollBar style implementations, e.g. native macOS
        // and Windows styles, may have no internal states
        const hasStates = control.states.length > 0 || control.contentItem.states.length > 0

        if (hasStates) {
            control.size = 0.5
            verify(control.state === "active" || control.contentItem.state === "active")

            control.size = 1.0
            verify(control.state !== "active" && control.contentItem.state !== "active")
        }
        control.policy = ScrollBar.AlwaysOff
        compare(control.visible, false)

        control.policy = ScrollBar.AlwaysOn
        compare(control.visible, true)
        if (hasStates) {
            verify(control.state === "active" || control.contentItem.state === "active")
        }
    }

    function test_overshoot() {
        let container = createTemporaryObject(flickable, testCase)
        verify(container)
        waitForRendering(container)

        let vertical = scrollBar.createObject(container, {size: 0.5})
        container.ScrollBar.vertical = vertical

        let horizontal = scrollBar.createObject(container, {size: 0.5})
        container.ScrollBar.horizontal = horizontal

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
        let control = createTemporaryObject(scrollBar, testCase)
        verify(control)

        compare(control.orientation, Qt.Vertical)
        compare(control.horizontal, false)
        compare(control.vertical, true)

        control.orientation = Qt.Horizontal
        compare(control.orientation, Qt.Horizontal)
        compare(control.horizontal, true)
        compare(control.vertical, false)
    }

    function test_flashing() {
        let control = createTemporaryObject(scrollBar, testCase, {size: 0.2})
        verify(control)

        let activeSpy = signalSpy.createObject(control, {target: control, signalName: "activeChanged"})
        verify(activeSpy.valid)

        compare(control.active, false)
        if (control.contentItem && control.contentItem.opacity > 0)
            // Slider handle is always visible in this style (Windows style)
            return

        if (control.contentItem)
            compare(control.contentItem.opacity, 0)
        if (control.background)
            compare(control.background.opacity, 0)

        control.increase()
        compare(control.position, 0.1)
        compare(control.active, false)
        compare(activeSpy.count, 2)
        if (control.contentItem)
            verify(control.contentItem.opacity > 0)
        if (control.background)
            verify(control.background.opacity > 0)
        if (control.contentItem)
            tryCompare(control.contentItem, "opacity", 0)
        if (control.background)
            tryCompare(control.background, "opacity", 0)

        control.decrease()
        compare(control.position, 0.0)
        compare(control.active, false)
        compare(activeSpy.count, 4)
        if (control.contentItem)
            verify(control.contentItem.opacity > 0)
        if (control.background)
            verify(control.background.opacity > 0)
        if (control.contentItem)
            tryCompare(control.contentItem, "opacity", 0)
        if (control.background)
            tryCompare(control.background, "opacity", 0)
    }

    function test_minimumSize() {
        let container = createTemporaryObject(flickable, testCase)
        verify(container)
        waitForRendering(container)

        let vertical = scrollBar.createObject(container, {minimumSize: 0.1})
        container.ScrollBar.vertical = vertical

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
        let vertical = createTemporaryObject(scrollBar, testCase, { height:200, orientation: Qt.Vertical, size: 0.5, position: 0.5 })
        verify(vertical)

        vertical.size = 0.8
        compare(vertical.position, 0.2)
        compare(vertical.visualPosition, 0.2)
        vertical.size = 0.5
        compare(vertical.position, 0.2)
        compare(vertical.visualPosition, 0.2)


        let horizontal = createTemporaryObject(scrollBar, testCase, { width:200, orientation: Qt.Horizontal, size: 0.5, position: 0.5 })
        verify(horizontal)

        horizontal.size = 0.8
        compare(horizontal.position, 0.2)
        compare(horizontal.visualPosition, 0.2)
        horizontal.size = 0.5
        compare(horizontal.position, 0.2)
        compare(horizontal.visualPosition, 0.2)
    }

    function test_setting_invalid_property_values() {
        let control = createTemporaryObject(scrollBar, testCase, {size: 2.0, minimumSize: -1.0})
        verify(control)

        // check that the values are within the expected range
        compare(control.size, 1.0)
        compare(control.minimumSize, 0)

        control.minimumSize = 2.0
        compare(control.minimumSize, 1.0)

        // test if setting NaN is prevented
        control.size = NaN
        control.minimumSize = NaN
        compare(control.size, 1.0)
        compare(control.minimumSize, 1.0)


        // test if setting float infinity is prevented
        control.size = Number.POSITIVE_INFINITY
        control.minimumSize = Number.POSITIVE_INFINITY
        compare(control.size, 1.0)
        compare(control.minimumSize, 1.0)

        let oldPosition = control.position;
        let oldStepSize = control.stepSize;

        control.position = NaN;
        control.stepSize = NaN;

        compare(oldPosition, control.position)
        compare(oldStepSize, control.stepSize)
    }

    function test_visualSizeChanged_is_emitted_by_setSize() {
        const control = createTemporaryObject(scrollBar, testCase, {size: 0.2})
        verify(control)

        const spy = signalSpy.createObject(control, {target: control, signalName: "visualSizeChanged"})
        verify(spy.valid)

        compare(control.visualSize, 0.2)

        control.size = 0.5

        compare(spy.count, 1)
        compare(control.visualSize, 0.5)
    }
}
