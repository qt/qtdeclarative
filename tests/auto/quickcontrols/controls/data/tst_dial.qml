// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest
import QtQuick.Controls

TestCase {
    id: testCase
    width: 450
    height: 450
    visible: true
    when: windowShown
    name: "Dial"

    Component {
        id: dialComponent
        Dial {
            width: 100
            height: 100
            anchors.centerIn: parent
        }
    }

    Component {
        id: signalSpy
        SignalSpy {}
    }

    function init() {
        // Fail on any warning that we don't expect.
        failOnWarning(/.?/)
    }

    function test_instance() {
        let dial = createTemporaryObject(dialComponent, testCase);
        verify(dial);
        compare(dial.value, 0.0);
        compare(dial.from, 0.0);
        compare(dial.to, 1.0);
        compare(dial.stepSize, 0.0);
        verify(dial.activeFocusOnTab);
        verify(!dial.pressed);
    }

    function test_value() {
        let dial = createTemporaryObject(dialComponent, testCase);
        verify(dial);
        compare(dial.value, 0.0);

        dial.value = 0.5;
        compare(dial.value, 0.5);

        dial.value = 1.0;
        compare(dial.value, 1.0);

        dial.value = -1.0;
        compare(dial.value, 0.0);

        dial.value = 2.0;
        compare(dial.value, 1.0);
    }

    function test_range() {
        let dial = createTemporaryObject(dialComponent, testCase);
        verify(dial);

        dial.from = 0;
        dial.to = 100;
        dial.value = 50;
        compare(dial.from, 0);
        compare(dial.to, 100);
        compare(dial.value, 50);
        compare(dial.position, 0.5);

        dial.value = 1000
        compare(dial.value, 100);
        compare(dial.position, 1);

        dial.value = -1
        compare(dial.value, 0);
        compare(dial.position, 0);

        dial.from = 25
        compare(dial.from, 25);
        compare(dial.value, 25);
        compare(dial.position, 0);

        dial.to = 75
        compare(dial.to, 75);
        compare(dial.value, 25);
        compare(dial.position, 0);

        dial.value = 50
        compare(dial.value, 50);
        compare(dial.position, 0.5);
    }

    function test_inverted() {
        let dial = createTemporaryObject(dialComponent, testCase, { from: 1.0, to: -1.0 });
        verify(dial);
        compare(dial.from, 1.0);
        compare(dial.to, -1.0);
        compare(dial.value, 0.0);
        compare(dial.position, 0.5);

        dial.value = 2.0;
        compare(dial.value, 1.0);
        compare(dial.position, 0.0);

        dial.value = -2.0;
        compare(dial.value, -1.0);
        compare(dial.position, 1.0);

        dial.value = 0.0;
        compare(dial.value, 0.0);
        compare(dial.position, 0.5);
    }

    SignalSpy {
        id: pressSpy
        signalName: "pressedChanged"
    }

    function test_pressed() {
        let dial = createTemporaryObject(dialComponent, testCase);
        verify(dial);

        pressSpy.target = dial;
        verify(pressSpy.valid);
        verify(!dial.pressed);

        mousePress(dial, dial.width / 2, dial.height / 2);
        verify(dial.pressed);
        compare(pressSpy.count, 1);

        mouseRelease(dial, dial.width / 2, dial.height / 2);
        verify(!dial.pressed);
        compare(pressSpy.count, 2);

        let touch = touchEvent(dial);
        touch.press(0).commit();
        verify(dial.pressed);
        compare(pressSpy.count, 3);

        touch.release(0).commit();
        verify(!dial.pressed);
        compare(pressSpy.count, 4);
    }

    SignalSpy {
        id: valueSpy
        signalName: "valueChanged"
    }

    function test_dragging_data() {
        return [
            { tag: "default", from: 0, to: 1, leftValue: 0.20, topValue: 0.5, rightValue: 0.8, bottomValue: 1.0, live: false },
            { tag: "scaled2", from: 0, to: 2, leftValue: 0.4, topValue: 1.0, rightValue: 1.6, bottomValue: 2.0, live: false },
            { tag: "scaled1", from: -1, to: 0, leftValue: -0.8, topValue: -0.5, rightValue: -0.2, bottomValue: 0.0, live: false },
            { tag: "live", from: 0, to: 1, leftValue: 0.20, topValue: 0.5, rightValue: 0.8, bottomValue: 1.0, live: true }
        ]
    }

    function test_dragging(data) {
        let dial = createTemporaryObject(dialComponent, testCase);
        verify(dial);

        dial.wrap = true;
        verify(dial.wrap);
        dial.from = data.from;
        dial.to = data.to;
        dial.live = data.live;

        valueSpy.target = dial;
        verify(valueSpy.valid);

        let moveSpy = createTemporaryObject(signalSpy, testCase, {target: dial, signalName: "moved"});
        verify(moveSpy.valid);

        let minimumExpectedValueCount = data.live ? 2 : 1;

        // drag to the left
        // we always add or subtract 1 to ensure we start the drag from the opposite side
        // of where we're dragging to, for more reliable tests
        mouseDrag(dial, dial.width / 2 + 1, dial.height / 2, -dial.width / 2, 0, Qt.LeftButton);
        fuzzyCompare(dial.value, data.leftValue, 0.1);
        verify(valueSpy.count >= minimumExpectedValueCount, "expected valueChanged to be emitted at least "
            + minimumExpectedValueCount + " time(s), but it was only emitted " + valueSpy.count + " time(s)");
        valueSpy.clear();
        verify(moveSpy.count > 0);
        moveSpy.clear();

        // drag to the top
        mouseDrag(dial, dial.width / 2, dial.height / 2 + 1, 0, -dial.height / 2, Qt.LeftButton);
        fuzzyCompare(dial.value, data.topValue, 0.1);
        verify(valueSpy.count >= minimumExpectedValueCount, "expected valueChanged to be emitted at least "
            + minimumExpectedValueCount + " time(s), but it was only emitted " + valueSpy.count + " time(s)");
        valueSpy.clear();
        verify(moveSpy.count > 0);
        moveSpy.clear();

        // drag to the right
        mouseDrag(dial, dial.width / 2 - 1, dial.height / 2, dial.width / 2, 0, Qt.LeftButton);
        fuzzyCompare(dial.value, data.rightValue, 0.1);
        verify(valueSpy.count >= minimumExpectedValueCount, "expected valueChanged to be emitted at least "
            + minimumExpectedValueCount + " time(s), but it was only emitted " + valueSpy.count + " time(s)");
        valueSpy.clear();
        verify(moveSpy.count > 0);
        moveSpy.clear();

        // drag to the bottom (* 0.6 to ensure we don't go over to the minimum position)
        mouseDrag(dial, dial.width / 2, dial.height / 2 - 1, 10, dial.height / 2, Qt.LeftButton);
        fuzzyCompare(dial.value, data.bottomValue, 0.1);
        verify(valueSpy.count >= minimumExpectedValueCount, "expected valueChanged to be emitted at least "
            + minimumExpectedValueCount + " time(s), but it was only emitted " + valueSpy.count + " time(s)");
        valueSpy.clear();
        verify(moveSpy.count > 0);
        moveSpy.clear();
    }

    function test_nonWrapping() {
        let dial = createTemporaryObject(dialComponent, testCase);
        verify(dial);

        compare(dial.wrap, false);
        dial.value = 0;

        // Ensure that dragging from bottom left to bottom right doesn't work.
        let yPos = dial.height * 0.75;
        mousePress(dial, dial.width * 0.25, yPos, Qt.LeftButton);
        let positionAtPress = dial.position;
        mouseMove(dial, dial.width * 0.5, yPos);
        verify(dial.position < positionAtPress);
        mouseMove(dial, dial.width * 0.75, yPos);
        verify(dial.position < positionAtPress);
        mouseRelease(dial, dial.width * 0.75, yPos, Qt.LeftButton);
        verify(dial.position < positionAtPress);

        // Try the same thing, but a bit higher.
        yPos = dial.height * 0.6;
        mousePress(dial, dial.width * 0.25, yPos, Qt.LeftButton);
        positionAtPress = dial.position;
        mouseMove(dial, dial.width * 0.5, yPos);
        verify(dial.position < positionAtPress);
        mouseMove(dial, dial.width * 0.75, yPos);
        verify(dial.position < positionAtPress);
        mouseRelease(dial, dial.width * 0.75, yPos, Qt.LeftButton);
        verify(dial.position < positionAtPress);

        // Going from below the center of the dial to above it should work (once it gets above the center).
        mousePress(dial, dial.width * 0.25, dial.height * 0.75, Qt.LeftButton);
        positionAtPress = dial.position;
        mouseMove(dial, dial.width * 0.5, dial.height * 0.6);
        verify(dial.position < positionAtPress);
        mouseMove(dial, dial.width * 0.5, dial.height * 0.4); //move over the top
        mouseMove(dial, dial.width * 0.75, dial.height * 0.6); //and back down again
        verify(dial.position > positionAtPress);
        mouseRelease(dial, dial.width * 0.75, dial.height * 0.3, Qt.LeftButton);
        verify(dial.position > positionAtPress);
    }

    function test_touch() {
        let dial = createTemporaryObject(dialComponent, testCase);
        verify(dial);

        let touch = touchEvent(dial);

        // Ensure that dragging from bottom left to bottom right doesn't work.
        let yPos = dial.height * 0.75;
        touch.press(0, dial, dial.width * 0.25, yPos).commit();
        let positionAtPress = dial.position;
        touch.move(0, dial, dial.width * 0.5, yPos).commit();
        compare(dial.position, positionAtPress);
        touch.move(0, dial, dial.width * 0.75, yPos).commit();
        compare(dial.position, positionAtPress);
        touch.release(0, dial, dial.width * 0.75, yPos).commit();
        compare(dial.position, positionAtPress);

        // Try the same thing, but a bit higher.
        yPos = dial.height * 0.6;
        touch.press(0, dial, dial.width * 0.25, yPos).commit();
        positionAtPress = dial.position;
        touch.move(0, dial, dial.width * 0.5, yPos).commit();
        compare(dial.position, positionAtPress);
        touch.move(0, dial, dial.width * 0.75, yPos).commit();
        compare(dial.position, positionAtPress);
        touch.release(0, dial, dial.width * 0.75, yPos).commit();
        compare(dial.position, positionAtPress);

        // Going from below the center of the dial to above it should work (once it gets above the center).
        touch.press(0, dial, dial.width * 0.25, dial.height * 0.75).commit();
        positionAtPress = dial.position;
        touch.move(0, dial, dial.width * 0.5, dial.height * 0.6).commit();
        compare(dial.position, positionAtPress);
        touch.move(0, dial, dial.width * 0.5, dial.height * 0.4).commit(); //move over the top
        touch.move(0, dial, dial.width * 0.75, dial.height * 0.6).commit(); //and back down again
        verify(dial.position > positionAtPress);
        touch.release(0, dial, dial.width * 0.75, dial.height * 0.3).commit();
        verify(dial.position > positionAtPress);
    }

    function test_multiTouch() {
        let dial1 = createTemporaryObject(dialComponent, testCase);
        verify(dial1);

        let touch = touchEvent(dial1);
        touch.press(0, dial1).commit().move(0, dial1, dial1.width / 4, dial1.height / 4).commit();
        compare(dial1.pressed, true);
        verify(dial1.position > 0.0);

        let pos1Before = dial1.position;

        // second touch point on the same control is ignored
        touch.stationary(0).press(1, dial1, 0, 0).commit()
        touch.stationary(0).move(1, dial1).commit()
        touch.stationary(0).release(1).commit()
        compare(dial1.pressed, true);
        compare(dial1.position, pos1Before);

        let dial2 = createTemporaryObject(dialComponent, testCase, {y: dial1.height});
        verify(dial2);

        // press the second dial
        touch.stationary(0).press(2, dial2, 0, 0).commit();
        compare(dial2.pressed, true);
        compare(dial2.position, 0.0);

        pos1Before = dial1.position;
        let pos2Before = dial2.position;

        // move both dials
        touch.move(0, dial1).move(2, dial2, dial2.width / 4, dial2.height / 4).commit();
        compare(dial1.pressed, true);
        verify(dial1.position !== pos1Before);
        compare(dial2.pressed, true);
        verify(dial2.position !== pos2Before);

        // release both dials
        touch.release(0, dial1).release(2, dial2).commit();
        compare(dial1.pressed, false);
        compare(dial1.value, dial1.position);
        compare(dial2.pressed, false);
        compare(dial2.value, dial2.position);
    }

    property Component focusTest: Component {
        FocusScope {
            signal receivedKeyPress

            Component.onCompleted: forceActiveFocus()
            anchors.fill: parent
            Keys.onPressed: receivedKeyPress()
        }
    }

    SignalSpy {
        id: parentEventSpy
    }

    function test_keyboardNavigation() {
        let dial = createTemporaryObject(dialComponent, testCase);
        verify(dial);

        let focusScope = createTemporaryObject(focusTest, testCase);
        verify(focusScope);

        let moveCount = 0;

        // Tests that we've accepted events that we're interested in.
        parentEventSpy.target = focusScope;
        parentEventSpy.signalName = "receivedKeyPress";

        let moveSpy = createTemporaryObject(signalSpy, testCase, {target: dial, signalName: "moved"});
        verify(moveSpy.valid);

        dial.parent = focusScope;
        compare(dial.activeFocusOnTab, true);
        compare(dial.value, 0);

        dial.focus = true;
        compare(dial.activeFocus, true);
        dial.stepSize = 0.1;

        keyClick(Qt.Key_Left);
        compare(parentEventSpy.count, 0);
        compare(moveSpy.count, moveCount);
        compare(dial.value, 0);

        let oldValue = 0.0;
        let keyPairs = [[Qt.Key_Left, Qt.Key_Right], [Qt.Key_Down, Qt.Key_Up]];
        for (let keyPairIndex = 0; keyPairIndex < 2; ++keyPairIndex) {
            for (let i = 1; i <= 10; ++i) {
                oldValue = dial.value;
                keyClick(keyPairs[keyPairIndex][1]);
                compare(parentEventSpy.count, 0);
                if (oldValue !== dial.value)
                    compare(moveSpy.count, ++moveCount);
                compare(dial.value, dial.stepSize * i);
            }

            compare(dial.value, dial.to);

            for (let i = 10; i > 0; --i) {
                oldValue = dial.value;
                keyClick(keyPairs[keyPairIndex][0]);
                compare(parentEventSpy.count, 0);
                if (oldValue !== dial.value)
                    compare(moveSpy.count, ++moveCount);
                compare(dial.value, dial.stepSize * (i - 1));
            }
        }

        dial.value = 0.5;

        keyClick(Qt.Key_Home);
        compare(parentEventSpy.count, 0);
        compare(moveSpy.count, ++moveCount);
        compare(dial.value, dial.from);

        keyClick(Qt.Key_Home);
        compare(parentEventSpy.count, 0);
        compare(moveSpy.count, moveCount);
        compare(dial.value, dial.from);

        keyClick(Qt.Key_End);
        compare(parentEventSpy.count, 0);
        compare(moveSpy.count, ++moveCount);
        compare(dial.value, dial.to);

        keyClick(Qt.Key_End);
        compare(parentEventSpy.count, 0);
        compare(moveSpy.count, moveCount);
        compare(dial.value, dial.to);
    }

    function test_snapMode_data(immediate) {
        return [
            { tag: "NoSnap", snapMode: Dial.NoSnap, from: 0, to: 2, values: [0, 0, 1], positions: [0, 0.5, 0.5] },
            { tag: "SnapAlways (0..2)", snapMode: Dial.SnapAlways, from: 0, to: 2, values: [0.0, 0.0, 1.0], positions: [0.0, 0.5, 0.5] },
            { tag: "SnapAlways (1..3)", snapMode: Dial.SnapAlways, from: 1, to: 3, values: [1.0, 1.0, 2.0], positions: [0.0, 0.5, 0.5] },
            { tag: "SnapAlways (-1..1)", snapMode: Dial.SnapAlways, from: -1, to: 1, values: [0.0, 0.0, 0.0], positions: [immediate ? 0.0 : 0.5, 0.5, 0.5] },
            { tag: "SnapAlways (1..-1)", snapMode: Dial.SnapAlways, from: 1, to: -1, values: [1.0, 1.0, 0.0], positions: [0.0, 0.5, 0.5] },
            { tag: "SnapOnRelease (0..2)", snapMode: Dial.SnapOnRelease, from: 0, to: 2, values: [0.0, 0.0, 1.0], positions: [0.0, 0.5, 0.5] },
            { tag: "SnapOnRelease (1..3)", snapMode: Dial.SnapOnRelease, from: 1, to: 3, values: [1.0, 1.0, 2.0], positions: [0.0, 0.5, 0.5] },
            { tag: "SnapOnRelease (-1..1)", snapMode: Dial.SnapOnRelease, from: -1, to: 1, values: [0.0, 0.0, 0.0], positions: [immediate ? 0.0 : 0.5, 0.5, 0.5] },
            { tag: "SnapOnRelease (1..-1)", snapMode: Dial.SnapOnRelease, from: 1, to: -1, values: [1.0, 1.0, 0.0], positions: [0.0, 0.5, 0.5] }
        ]
    }

    function test_snapMode_mouse_data() {
        return test_snapMode_data(true)
    }

    function test_snapMode_mouse(data) {
        let dial = createTemporaryObject(dialComponent, testCase, {live: false});
        verify(dial);

        dial.snapMode = data.snapMode;
        dial.from = data.from;
        dial.to = data.to;
        dial.stepSize = 0.2;

        let fuzz = 0.055;

        mousePress(dial, dial.width * 0.25, dial.height * 0.75);
        fuzzyCompare(dial.value, data.values[0], fuzz);
        fuzzyCompare(dial.position, data.positions[0], fuzz);

        mouseMove(dial, dial.width * 0.5, dial.height * 0.25);
        fuzzyCompare(dial.value, data.values[1], fuzz);
        fuzzyCompare(dial.position, data.positions[1], fuzz);

        mouseRelease(dial, dial.width * 0.5, dial.height * 0.25);
        fuzzyCompare(dial.value, data.values[2], fuzz);
        fuzzyCompare(dial.position, data.positions[2], fuzz);
    }

    function test_snapMode_touch_data() {
        return test_snapMode_data(false)
    }

    function test_snapMode_touch(data) {
        let dial = createTemporaryObject(dialComponent, testCase, {live: false});
        verify(dial);

        dial.snapMode = data.snapMode;
        dial.from = data.from;
        dial.to = data.to;
        dial.stepSize = 0.2;

        let fuzz = 0.05;

        let touch = touchEvent(dial);
        touch.press(0, dial, dial.width * 0.25, dial.height * 0.75).commit()
        compare(dial.value, data.values[0]);
        compare(dial.position, data.positions[0]);

        touch.move(0, dial, dial.width * 0.5, dial.height * 0.25).commit();
        fuzzyCompare(dial.value, data.values[1], fuzz);
        fuzzyCompare(dial.position, data.positions[1], fuzz);

        touch.release(0, dial, dial.width * 0.5, dial.height * 0.25).commit();
        fuzzyCompare(dial.value, data.values[2], fuzz);
        fuzzyCompare(dial.position, data.positions[2], fuzz);
    }

    function test_wheel_data() {
        return [
            { tag: "horizontal", dx: 120, dy: 0 },
            { tag: "vertical", dx: 0, dy: 120 }
        ]
    }

    function test_wheel(data) {
        let control = createTemporaryObject(dialComponent, testCase, {wheelEnabled: true})
        verify(control)

        compare(control.value, 0.0)

        mouseWheel(control, control.width / 2, control.height / 2, data.dx, data.dy)
        compare(control.value, 0.1)
        compare(control.position, 0.1)

        control.stepSize = 0.2

        mouseWheel(control, control.width / 2, control.height / 2, data.dx, data.dy)
        compare(control.value, 0.3)
        compare(control.position, 0.3)

        control.stepSize = 10.0

        mouseWheel(control, control.width / 2, control.height / 2, -data.dx, -data.dy)
        compare(control.value, 0.0)
        compare(control.position, 0.0)

        control.to = 10.0
        control.stepSize = 5.0

        mouseWheel(control, control.width / 2, control.height / 2, data.dx, data.dy)
        compare(control.value, 5.0)
        compare(control.position, 0.5)

        mouseWheel(control, control.width / 2, control.height / 2, 0.5 * data.dx, 0.5 * data.dy)
        compare(control.value, 7.5)
        compare(control.position, 0.75)

        mouseWheel(control, control.width / 2, control.height / 2, -data.dx, -data.dy)
        compare(control.value, 2.5)
        compare(control.position, 0.25)
    }

    function test_nullHandle() {
        let control = createTemporaryObject(dialComponent, testCase)
        verify(control)

        control.handle = null

        mousePress(control)
        verify(control.pressed, true)

        mouseRelease(control)
        compare(control.pressed, false)
    }

    function move(inputEventType, control, x, y) {
        if (inputEventType === "mouseInput") {
            mouseMove(control, x, y);
        } else {
            let touch = touchEvent(control);
            touch.move(0, control, x, y).commit();
        }
    }

    function press(inputEventType, control, x, y) {
        if (inputEventType === "mouseInput") {
            mousePress(control, x, y);
        } else {
            let touch = touchEvent(control);
            touch.press(0, control, x, y).commit();
        }
    }

    function release(inputEventType, control, x, y) {
        if (inputEventType === "mouseInput") {
            mouseRelease(control, x, y);
        } else {
            let touch = touchEvent(control);
            touch.release(0, control, x, y).commit();
        }
    }

    function test_horizontalAndVertical_data() {
        let data = [
            { eventType: "mouseInput", inputMode: Dial.Vertical, moveToX: 0.5, moveToY: 0.25, expectedPosition: 0.125 },
            // Horizontal movement should have no effect on a vertical dial.
            { eventType: "mouseInput", inputMode: Dial.Vertical, moveToX: 2.0, moveToY: 0.25, expectedPosition: 0.125 },
            { eventType: "mouseInput", inputMode: Dial.Vertical, moveToX: 0.5, moveToY: 0.0, expectedPosition: 0.25 },
            { eventType: "mouseInput", inputMode: Dial.Vertical, moveToX: 0.5, moveToY: -1.5, expectedPosition: 1.0 },
            // Going above the drag area shouldn't make the position higher than 1.0.
            { eventType: "mouseInput", inputMode: Dial.Vertical, moveToX: 0.5, moveToY: -2.0, expectedPosition: 1.0 },
            // Try to decrease the position by moving the mouse down.
            // The dial's position is 0 before the press event, so nothing should happen.
            { eventType: "mouseInput", inputMode: Dial.Vertical, moveToX: 0.5, moveToY: 1.25, expectedPosition: 0.0 },

            { eventType: "mouseInput", inputMode: Dial.Horizontal, moveToX: 0.75, moveToY: 0.5, expectedPosition: 0.125 },
            // Vertical movement should have no effect on a horizontal dial.
            { eventType: "mouseInput", inputMode: Dial.Horizontal, moveToX: 0.75, moveToY: 2.0, expectedPosition: 0.125 },
            { eventType: "mouseInput", inputMode: Dial.Horizontal, moveToX: 1.0, moveToY: 0.5, expectedPosition: 0.25 },
            { eventType: "mouseInput", inputMode: Dial.Horizontal, moveToX: 1.5, moveToY: 0.5, expectedPosition: 0.5 },
            { eventType: "mouseInput", inputMode: Dial.Horizontal, moveToX: 2.5, moveToY: 0.5, expectedPosition: 1.0 },
            // Going above the drag area shouldn't make the position higher than 1.0.
            { eventType: "mouseInput", inputMode: Dial.Horizontal, moveToX: 2.525, moveToY: 0.5, expectedPosition: 1.0 },
            // Try to decrease the position by moving the mouse to the left.
            // The dial's position is 0 before the press event, so nothing should happen.
            { eventType: "mouseInput", inputMode: Dial.Vertical, moveToX: 0.25, moveToY: 0.5, expectedPosition: 0.0 }
        ];

        // Do the same tests for touch by copying the mouse tests and adding them to the end of the array.
        let mouseTestCount = data.length;
        for (let i = mouseTestCount; i < mouseTestCount * 2; ++i) {
            // Shallow-copy the object.
            data[i] = JSON.parse(JSON.stringify(data[i - mouseTestCount]));
            data[i].eventType = "touchInput";
        }

        for (let i = 0; i < data.length; ++i) {
            let row = data[i];
            row.tag = "eventType=" + row.eventType + ", "
                    + "inputMode=" + (row.inputMode === Dial.Vertical ? "Vertical" : "Horizontal") + ", "
                    + "moveToX=" + row.moveToX + ", moveToY=" + row.moveToY + ", "
                    + "expectedPosition=" + row.expectedPosition;
        }

        return data;
    }

    function test_horizontalAndVertical(data) {
        let control = createTemporaryObject(dialComponent, testCase, { inputMode: data.inputMode });
        verify(control);

        press(data.eventType, control);
        compare(control.pressed, true);
        // The position shouldn't change until the mouse has actually moved.
        compare(control.position, 0);

        move(data.eventType, control, control.width * data.moveToX, control.width * data.moveToY);
        compare(control.position, data.expectedPosition);

        release(data.eventType, control, control.width * data.moveToX, control.width * data.moveToY);
        compare(control.pressed, false);
        compare(control.position, data.expectedPosition);
    }

    function test_integerStepping() {
        let dial = createTemporaryObject(dialComponent, testCase)
        verify(dial)

        dial.from = 1
        dial.to = 8
        dial.stepSize = 1

        for (let i = 1; i < 8; ++i) {
            // compare as strings to avoid a fuzzy compare; we want an exact match
            compare(""+dial.value, ""+1)
            keyClick(Qt.Key_Right)
        }
    }

    function test_startEndAngle_data() {
        return [
            {
                tag: "Default wrap", startAngle: -140, endAngle: 140, from: 0, to: 1, wrap: true,
                x:      [0.49, 0.25, 0.5, 0.75, 0.51, 0.49, 0.51],
                y:      [0.99, 0.5, 0.01, 0.5, 0.99, 0.99, 0.99],
                values: [0.0, 0.5-0.32, 0.5, 0.5+0.32, 1.0, 0.0, 1.0], //140/90*0.5 = 0.32
                angles: [-140.0, -90.0, 0.0, 90.0, 140.0, -140.0, 140.0],
                wrapClockwise: 1,
                wrapCounterClockwise: 1
            },
            {
                tag: "-30..30 wrap", startAngle: -30, endAngle: 30, from: 0, to: 1, wrap: true,
                x:      [0.49, 0.25, 0.5, 0.75, 0.51, 0.49, 0.51],
                y:      [0.99, 0.5, 0.01, 0.5, 0.99, 0.99, 0.99],
                values: [0.0, 0.0, 0.5, 1.0, 1.0, 0.0, 1.0],
                angles: [-30.0, -30.0, 0.0, 30.0, 30.0, -30.0, 30.0],
                wrapClockwise: 0, //no wrap if angle < 180
                wrapCounterClockwise: 0
            },
            {
                tag: "-180..180 wrap", startAngle: -180, endAngle: 180, from: 0, to: 1, wrap: true,
                x:      [0.49, 0.25, 0.5, 0.75, 0.51, 0.49, 0.51],
                y:      [0.99, 0.5, 0.01, 0.5, 0.99, 0.99, 0.99],
                values: [0.0, 0.25, 0.5, 0.75, 1.0, 0.0, 1.0],
                angles: [-180.0, -90.0, 0.0, 90.0, 180.0, -180.0, 180.0],
                wrapClockwise: 1,
                wrapCounterClockwise: 1
            },
            {
                tag: "90..360 wrap", startAngle: 90, endAngle: 360, from: 0, to: 1, wrap: true,
                x:      [0.49, 0.25, 0.5, 0.75, 0.51, 0.49, 0.5],
                y:      [0.99, 0.5, 0.01, 0.5, 0.99, 0.99, 0.01],
                values: [0.33, 0.66, 1.0, 0.0, 0.33, 0.33, 1.0],
                angles: [180.0, 270.0, 360.0, 90.0, 180.0, 180.0, 360.0],
                wrapClockwise: 1,
                wrapCounterClockwise: 1
            },
            {
                tag: "90..450 wrap", startAngle: 90, endAngle: 450, from: 0, to: 1, wrap: true,
                x:      [0.49, 0.25, 0.5, 0.75, 0.75, 0.51, 0.49, 0.75, 0.75],
                y:      [0.99, 0.5, 0.01, 0.49, 0.501, 0.99, 0.99, 0.49, 0.501],
                values: [0.25, 0.5, 0.75, 1.0, 0.0, 0.25, 0.25, 1.0, 0.0],
                angles: [180.0, 270.0, 360.0, 450.0, 90.0, 180.0, 180.0, 450.0, 90.0],
                wrapClockwise: 2,
                wrapCounterClockwise: 1
            },
            {
                tag: "Default nowrap", startAngle: -140, endAngle: 140, from: 0, to: 1, wrap: false,
                x:      [0.49, 0.25, 0.5, 0.75, 0.51, 0.49],
                y:      [0.99, 0.5, 0.01, 0.5, 0.99, 0.99],
                values: [0.0, 0.5-0.32, 0.5, 0.5+0.32, 1.0, 1.0], //140/90*0.5 = 0.32
                angles: [-140.0, -90.0, 0.0, 90.0, 140.0, 140.0],
                wrapClockwise: 0,
                wrapCounterClockwise: 0
            },
            {
                tag: "-30..30 nowrap", startAngle: -30, endAngle: 30, from: 0, to: 1, wrap: false,
                x:      [0.49, 0.25, 0.5, 0.75, 0.51, 0.49],
                y:      [0.99, 0.5, 0.01, 0.5, 0.99, 0.99],
                values: [0.0, 0.0, 0.5, 1.0, 1.0, 1.0],
                angles: [-30.0, -30.0, 0.0, 30.0, 30.0, 30.0],
                wrapClockwise: 0,
                wrapCounterClockwise: 0
            },
            {
                tag: "-180..180 nowrap", startAngle: -180, endAngle: 180, from: 0, to: 1, wrap: false,
                x:      [0.49, 0.25, 0.5, 0.75, 0.51, 0.49],
                y:      [0.99, 0.5, 0.01, 0.5, 0.99, 0.99],
                values: [0.0, 0.25, 0.5, 0.75, 1.0, 1.0],
                angles: [-180.0, -90.0, 0.0, 90.0, 180.0, 180.0],
                wrapClockwise: 0,
                wrapCounterClockwise: 0
            },
            {
                tag: "90..360 nowrap", startAngle: 90, endAngle: 360, from: 0, to: 1, wrap: false,
                x:      [0.49, 0.25, 0.5, 0.75, 0.51, 0.49],
                y:      [0.99, 0.5, 0.01, 0.5, 0.99, 0.99],
                values: [0.33, 0.66, 1.0, 1.0, 1.0, 1.0],
                angles: [180.0, 270.0, 360.0, 360.0, 360.0, 360.0],
                wrapClockwise: 0,
                wrapCounterClockwise: 0
            }
        ]
    }

    function test_startEndAngle(data) {
        let dial = createTemporaryObject(dialComponent, testCase)
        verify(dial)

        dial.startAngle = data.startAngle
        dial.endAngle = data.endAngle
        dial.from = data.from
        dial.to = data.to
        //Give a defined start in case wrap = true
        dial.value = data.values[0]
        dial.wrap = data.wrap

        compare(dial.startAngle, data.startAngle)
        compare(dial.endAngle, data.endAngle)

        let wrappedSpy = signalSpy.createObject(dial, {target: dial, signalName: "wrapped"})
        verify(wrappedSpy.valid)

        for (let i = 0; i < data.x.length; i++) {
            mousePress(dial, dial.width * data.x[i], dial.height * 0.5 + dial.width * ( data.y[i] - 0.5))
            fuzzyCompare(dial.angle, data.angles[i], 3.0)
            fuzzyCompare(dial.value, data.values[i], 0.1)
        }

        let clockwiseCount = 0
        let counterClockwiseCount = 0
        for (let i = 0; i < wrappedSpy.count; i++) {
            if (wrappedSpy.signalArguments[i][0] == 0)
                clockwiseCount++;
            else
                counterClockwiseCount++;
        }

        compare(clockwiseCount, data.wrapClockwise)
        compare(counterClockwiseCount, data.wrapCounterClockwise)
    }

    function test_startEndAngleWarnings(data) {
        let dial = createTemporaryObject(dialComponent, testCase)
        verify(dial)

        dial.startAngle = -180.
        dial.endAngle = 180.

        //provoke warning
        ignoreWarning(new RegExp("Changing endAngle to avoid overlaps"))
        dial.startAngle = -270.
        dial.endAngle = 90.

        compare(dial.startAngle, -270.)
        compare(dial.endAngle, 90.)


        dial.startAngle = -180.
        dial.endAngle = 180.

        //provoke warning
        ignoreWarning(new RegExp("Changing startAngle to avoid overlaps"))
        dial.endAngle = 270.
        dial.startAngle = -90.

        compare(dial.startAngle, -90.)
        compare(dial.endAngle, 270.)

        {
            // Should not warn since we delay the setting of start and end angles to avoid
            // binding order evaluation conflicts.
            let dial = createTemporaryObject(dialComponent, testCase, { startAngle: -10, endAngle: 300 })
            verify(dial)
            compare(dial.startAngle, -10.)
            compare(dial.endAngle, 300.)
        }
    }

    function test_notSquareGeometry() {
        let dial = createTemporaryObject(dialComponent, testCase)
        verify(dial);
        if (!dial.handle) {
            skip("Test cannot run on styles where handle == null (macOS style)")
        }
        dial.from = 0
        dial.to = 1
        dial.live = true
        dial.wrap = true
        dial.startAngle = -180
        dial.endAngle = 180

        // Dial input handling always assumes that the dial is in the *center*.
        // Instantiate a Dial with a geometries of 400x100 and then 100x400
        // Some styles always could wrongly align the Dial background and handle in the topLeft
        // corner. Pressing in the handle would cause the Dial to move because the dial
        // assumes that the "Dial circle" is center aligned in its geometry.
        for (let pass = 0; pass < 2; ++pass) {
            if (pass === 0) {
                dial.width = testCase.width
                dial.height = 100
            } else {
                dial.width = 100
                dial.height = testCase.height
            }

            let val = pass * 0.25
            dial.value = val
            // find coordinates in the middle of the handle
            let pt2 = dial.mapFromItem(dial.handle, dial.handle.width/2, dial.handle.height/2)
            // press the knob in the middle. It shouldn't move (except from due to rounding errors)
            mousePress(dial, pt2.x, pt2.y)
            fuzzyCompare(dial.value, val, 0.1)
        }
    }

}
