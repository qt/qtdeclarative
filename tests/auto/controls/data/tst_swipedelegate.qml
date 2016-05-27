/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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

import QtQuick 2.6
import QtTest 1.0
import QtQuick.Controls 2.0

TestCase {
    id: testCase
    width: 200
    height: 200
    visible: true
    when: windowShown
    name: "SwipeDelegate"

    readonly property int dragDistance: Math.max(20, Qt.styleHints.startDragDistance + 5)

    Component {
        id: greenLeftComponent

        Rectangle {
            objectName: "leftItem"
            anchors.fill: parent
            color: "green"
        }
    }

    Component {
        id: redRightComponent

        Rectangle {
            objectName: "rightItem"
            anchors.fill: parent
            color: "red"
        }
    }

    Component {
        id: swipeDelegateComponent

        SwipeDelegate {
            id: swipeDelegate
            text: "SwipeDelegate"
            width: 150
            swipe.left: greenLeftComponent
            swipe.right: redRightComponent
        }
    }

    function test_defaults() {
        var control = swipeDelegateComponent.createObject(testCase);
        verify(control);

        compare(control.baselineOffset, control.contentItem.y + control.contentItem.baselineOffset);
        compare(control.swipe.position, 0);
        verify(!control.pressed);
        verify(!control.swipe.complete);

        control.destroy();
    }

    Component {
        id: itemComponent

        Item {}
    }

    // Assumes that the delegate is smaller than the width of the control.
    function swipe(control, from, to) {
        // Sanity check.
        compare(control.swipe.position, from);

        var distance = (to - from) * control.width;

        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton);
        mouseMove(control, control.width / 2 + distance, control.height / 2, Qt.LeftButton);
        mouseRelease(control, control.width / 2 + distance, control.height / 2, Qt.LeftButton);
        compare(control.swipe.position, to);

        if (control.swipe.position === -1.0) {
            if (control.swipe.right)
                verify(control.swipe.rightItem);
            else if (control.swipe.behind)
                verify(control.swipe.behindItem);
        } else if (control.swipe.position === 1.0) {
            if (control.swipe.left)
                verify(control.swipe.leftItem);
            else if (control.swipe.behind)
                verify(control.swipe.behindItem);
        }
    }

    function test_settingDelegates() {
        var control = swipeDelegateComponent.createObject(testCase);
        verify(control);

        ignoreWarning(Qt.resolvedUrl("tst_swipedelegate.qml") +
            ":78:9: QML SwipeDelegate: cannot set both behind and left/right properties")
        control.swipe.behind = itemComponent;

        // Shouldn't be any warnings when unsetting delegates.
        control.swipe.left = null;
        compare(control.swipe.leftItem, null);

        // right is still set.
        ignoreWarning(Qt.resolvedUrl("tst_swipedelegate.qml") +
            ":78:9: QML SwipeDelegate: cannot set both behind and left/right properties")
        control.swipe.behind = itemComponent;

        control.swipe.right = null;
        compare(control.swipe.rightItem, null);

        control.swipe.behind = itemComponent;

        ignoreWarning(Qt.resolvedUrl("tst_swipedelegate.qml") +
            ":78:9: QML SwipeDelegate: cannot set both behind and left/right properties")
        control.swipe.left = itemComponent;

        ignoreWarning(Qt.resolvedUrl("tst_swipedelegate.qml") +
            ":78:9: QML SwipeDelegate: cannot set both behind and left/right properties")
        control.swipe.right = itemComponent;

        control.swipe.behind = null;
        control.swipe.left = greenLeftComponent;
        control.swipe.right = redRightComponent;

        // Test that the user is warned when attempting to set or unset left or
        // right item while they're exposed.
        // First, try the left item.
        swipe(control, 0.0, 1.0);

        var oldLeft = control.swipe.left;
        var oldLeftItem = control.swipe.leftItem;
        ignoreWarning(Qt.resolvedUrl("tst_swipedelegate.qml") +
            ":78:9: QML SwipeDelegate: left/right/behind properties may only be set when swipe.position is 0")
        control.swipe.left = null;
        compare(control.swipe.left, oldLeft);
        compare(control.swipe.leftItem, oldLeftItem);

        // Try the same thing with the right item.
        swipe(control, 1.0, -1.0);

        var oldRight = control.swipe.right;
        var oldRightItem = control.swipe.rightItem;
        ignoreWarning(Qt.resolvedUrl("tst_swipedelegate.qml") +
            ":78:9: QML SwipeDelegate: left/right/behind properties may only be set when swipe.position is 0")
        control.swipe.right = null;
        compare(control.swipe.right, oldRight);
        compare(control.swipe.rightItem, oldRightItem);

        // Return to the default position.
        swipe(control, -1.0, 0.0);

        tryCompare(control.background, "x", 0, 1000);

        // Try the same thing with the behind item.
        control.swipe.left = null;
        verify(!control.swipe.left);
        verify(!control.swipe.leftItem);
        control.swipe.right = null;
        verify(!control.swipe.right);
        verify(!control.swipe.rightItem);
        control.swipe.behind = greenLeftComponent;
        verify(control.swipe.behind);
        verify(!control.swipe.behindItem);

        swipe(control, 0.0, 1.0);

        var oldBehind = control.swipe.behind;
        var oldBehindItem = control.swipe.behindItem;
        ignoreWarning(Qt.resolvedUrl("tst_swipedelegate.qml") +
            ":78:9: QML SwipeDelegate: left/right/behind properties may only be set when swipe.position is 0")
        control.swipe.behind = null;
        compare(control.swipe.behind, oldBehind);
        compare(control.swipe.behindItem, oldBehindItem);

        control.destroy();
    }

    SignalSequenceSpy {
        id: mouseSignalSequenceSpy
        signals: ["pressed", "released", "canceled", "clicked", "doubleClicked", "pressedChanged"]
    }

    function test_swipe() {
        var control = swipeDelegateComponent.createObject(testCase);
        verify(control);

        var overDragDistance = dragDistance * 1.1;

        mouseSignalSequenceSpy.target = control;
        mouseSignalSequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true }], "pressed"];
        mousePress(control, control.width / 2, control.height / 2);
        verify(control.pressed);
        compare(control.swipe.position, 0.0);
        verify(!control.swipe.complete);
        verify(mouseSignalSequenceSpy.success);
        verify(!control.swipe.leftItem);
        verify(!control.swipe.rightItem);

        // Drag to the right so that leftItem is created and visible.
        mouseMove(control, control.width / 2 + overDragDistance, control.height / 2);
        verify(control.pressed);
        compare(control.swipe.position, overDragDistance / control.width);
        verify(!control.swipe.complete);
        verify(control.swipe.leftItem);
        verify(control.swipe.leftItem.visible);
        compare(control.swipe.leftItem.parent, control);
        compare(control.swipe.leftItem.objectName, "leftItem");
        verify(!control.swipe.rightItem);

        // Go back to 0.
        mouseMove(control, control.width / 2, control.height / 2);
        verify(control.pressed);
        compare(control.swipe.position, 0.0);
        verify(!control.swipe.complete);
        verify(control.swipe.leftItem);
        verify(control.swipe.leftItem.visible);
        compare(control.swipe.leftItem.parent, control);
        compare(control.swipe.leftItem.objectName, "leftItem");
        verify(!control.swipe.rightItem);

        // Try the other direction. The right item should be created and visible,
        // and the left item should be hidden.
        mouseMove(control, control.width / 2 - overDragDistance, control.height / 2);
        verify(control.pressed);
        compare(control.swipe.position, -overDragDistance / control.width);
        verify(!control.swipe.complete);
        verify(control.swipe.leftItem);
        verify(!control.swipe.leftItem.visible);
        verify(control.swipe.rightItem);
        verify(control.swipe.rightItem.visible);
        compare(control.swipe.rightItem.parent, control);
        compare(control.swipe.rightItem.objectName, "rightItem");

        // Now release outside the right edge of the control.
        mouseMove(control, control.width * 1.1, control.height / 2);
        verify(control.pressed);
        compare(control.swipe.position, 0.6);
        verify(!control.swipe.complete);
        verify(control.swipe.leftItem);
        verify(control.swipe.leftItem.visible);
        verify(control.swipe.rightItem);
        verify(!control.swipe.rightItem.visible);

        mouseSignalSequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false }], "released", "clicked"];
        mouseRelease(control, control.width / 2, control.height / 2);
        verify(!control.pressed);
        compare(control.swipe.position, 1.0);
        verify(control.swipe.complete);
        verify(mouseSignalSequenceSpy.success);
        verify(control.swipe.leftItem);
        verify(control.swipe.leftItem.visible);
        verify(control.swipe.rightItem);
        verify(!control.swipe.rightItem.visible);
        tryCompare(control.contentItem, "x", control.width + control.leftPadding);

        // Swiping from the right and releasing early should return position to 1.0.
        mouseSignalSequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true }], "pressed"];
        mousePress(control, control.width / 2, control.height / 2);
        verify(control.pressed);
        compare(control.swipe.position, 1.0);
        // complete should still be true, because we haven't moved yet, and hence
        // haven't started grabbing behind's mouse events.
        verify(control.swipe.complete);
        verify(mouseSignalSequenceSpy.success);

        mouseMove(control, control.width / 2 - overDragDistance, control.height / 2);
        verify(control.pressed);
        verify(!control.swipe.complete);
        compare(control.swipe.position, 1.0 - overDragDistance / control.width);

        mouseSignalSequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false }], "released", "clicked"];
        mouseRelease(control, control.width * 0.4, control.height / 2);
        verify(!control.pressed);
        compare(control.swipe.position, 1.0);
        verify(control.swipe.complete);
        verify(mouseSignalSequenceSpy.success);
        tryCompare(control.contentItem, "x", control.width + control.leftPadding);

        // Swiping from the right and releasing should return contents to default position.
        mouseSignalSequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true }], "pressed"];
        mousePress(control, control.width / 2, control.height / 2);
        verify(control.pressed);
        compare(control.swipe.position, 1.0);
        verify(control.swipe.complete);
        verify(mouseSignalSequenceSpy.success);

        mouseMove(control, control.width * -0.1, control.height / 2);
        verify(control.pressed);
        verify(!control.swipe.complete);
        compare(control.swipe.position, 0.4);

        mouseSignalSequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false }], "released", "clicked"];
        mouseRelease(control, control.width * -0.1, control.height / 2);
        verify(!control.pressed);
        compare(control.swipe.position, 0.0);
        verify(!control.swipe.complete);
        verify(mouseSignalSequenceSpy.success);
        tryCompare(control.contentItem, "x", control.leftPadding);

        control.destroy();
    }

    function test_swipeVelocity_data() {
        return [
            { tag: "positive velocity", direction: 1 },
            { tag: "negative velocity", direction: -1 }
        ];
    }

    function test_swipeVelocity(data) {
        skip("QTBUG-52003");

        var control = swipeDelegateComponent.createObject(testCase);
        verify(control);

        var distance = dragDistance * 1.1;
        if (distance >= control.width / 2)
            skip("This test requires a startDragDistance that is less than half the width of the control");

        distance *= data.direction;

        mouseSignalSequenceSpy.target = control;
        mouseSignalSequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true }], "pressed"];
        mousePress(control, control.width / 2, control.height / 2);
        verify(control.pressed);
        compare(control.swipe.position, 0.0);
        verify(!control.swipe.complete);
        verify(mouseSignalSequenceSpy.success);
        verify(!control.swipe.leftItem);
        verify(!control.swipe.rightItem);

        // Swipe quickly to the side over a distance that is longer than the drag threshold,
        // quicker than the expose velocity threshold, but shorter than the halfway mark.
        mouseMove(control, control.width / 2 + distance, control.height / 2);
        verify(control.pressed);
        compare(control.swipe.position, distance / control.width);
        verify(control.swipe.position < 0.5);
        verify(!control.swipe.complete);

        var expectedVisibleItem;
        var expectedVisibleObjectName;
        var expectedHiddenItem;
        var expectedContentItemX;
        if (distance > 0) {
            expectedVisibleObjectName = "leftItem";
            expectedVisibleItem = control.swipe.leftItem;
            expectedHiddenItem = control.swipe.rightItem;
            expectedContentItemX = control.width + control.leftPadding;
        } else {
            expectedVisibleObjectName = "rightItem";
            expectedVisibleItem = control.swipe.rightItem;
            expectedHiddenItem = control.swipe.leftItem;
            expectedContentItemX = -control.width + control.leftPadding;
        }
        verify(expectedVisibleItem);
        verify(expectedVisibleItem.visible);
        compare(expectedVisibleItem.parent, control);
        compare(expectedVisibleItem.objectName, expectedVisibleObjectName);
        verify(!expectedHiddenItem);

        mouseSignalSequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false }], "released", "clicked"];
        // Add a delay to ensure that the release event doesn't happen too quickly,
        // and hence that the second timestamp isn't zero (can happen with e.g. release builds).
        mouseRelease(control, control.width / 2 + distance, control.height / 2, Qt.LeftButton, Qt.NoModifier, 30);
        verify(!control.pressed);
        compare(control.swipe.position, data.direction);
        verify(control.swipe.complete);
        verify(mouseSignalSequenceSpy.success);
        verify(expectedVisibleItem);
        verify(expectedVisibleItem.visible);
        verify(!expectedHiddenItem);
        tryCompare(control.contentItem, "x", expectedContentItemX);

        control.destroy();
    }

    Component {
        id: swipeDelegateWithButtonComponent
        SwipeDelegate {
            text: "SwipeDelegate"
            width: 150
            swipe.right: Button {
                width: parent.width
                height: parent.height
                text: "Boo!"
            }
        }
    }

    function test_eventsToLeftAndRight() {
        var control = swipeDelegateWithButtonComponent.createObject(testCase);
        verify(control);

        // The button should be pressed instead of the SwipeDelegate.
        mouseDrag(control, control.width / 2, control.height / 2,  -control.width, 0);
        verify(!control.pressed);
        compare(control.swipe.position, -1.0);
        verify(control.swipe.rightItem);
        verify(control.swipe.rightItem.visible);
        compare(control.swipe.rightItem.parent, control);

        mousePress(control, control.width / 2, control.height / 2);
        verify(!control.pressed);
        var button = control.swipe.rightItem;
        verify(button.pressed);

        mouseRelease(control, control.width / 2, control.height / 2);
        verify(!button.pressed);

        // Returning back to a position of 0 and pressing on the control should
        // result in the control being pressed.
        mouseDrag(control, control.width / 2, control.height / 2, control.width * 0.6, 0);
        compare(control.swipe.position, 0);
        mousePress(control, control.width / 2, control.height / 2);
        verify(control.pressed);
        verify(!button.pressed);
        mouseRelease(control, control.width / 2, control.height / 2);
        verify(!control.pressed);

        control.destroy();
    }

    function test_mouseButtons() {
        var control = swipeDelegateComponent.createObject(testCase);
        verify(control);

        // click
        mouseSignalSequenceSpy.target = control;
        mouseSignalSequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true }], "pressed"];
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton);
        compare(control.pressed, true);

        verify(mouseSignalSequenceSpy.success);

        mouseSignalSequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false }], "released", "clicked"];
        mouseRelease(control, control.width / 2, control.height / 2, Qt.LeftButton);
        compare(control.pressed, false);
        verify(mouseSignalSequenceSpy.success);

        // right button
        mouseSignalSequenceSpy.expectedSequence = [];
        mousePress(control, control.width / 2, control.height / 2, Qt.RightButton);
        compare(control.pressed, false);

        mouseRelease(control, control.width / 2, control.height / 2, Qt.RightButton);
        compare(control.pressed, false);
        verify(mouseSignalSequenceSpy.success);

        // double click
        mouseSignalSequenceSpy.expectedSequence = [
            ["pressedChanged", { "pressed": true }],
            "pressed",
            ["pressedChanged", { "pressed": false }],
            "released",
            "clicked",
            ["pressedChanged", { "pressed": true }],
            "pressed",
            "doubleClicked",
            ["pressedChanged", { "pressed": false }],
            "released",
            "clicked"
        ];
        mouseDoubleClickSequence(control, control.width / 2, control.height / 2, Qt.LeftButton);
        verify(mouseSignalSequenceSpy.success);

        control.destroy();
    }

    Component {
        id: removableDelegatesComponent

        ListView {
            id: listView
            width: 100
            height: 120

            model: ListModel {
                ListElement { name: "Apple" }
                ListElement { name: "Orange" }
                ListElement { name: "Pear" }
            }

            delegate: SwipeDelegate {
                id: rootDelegate
                text: modelData
                width: listView.width

                onClicked: if (swipe.complete) ListView.view.model.remove(index)

                property alias removeAnimation: onRemoveAnimation

                ListView.onRemove: SequentialAnimation {
                    id: onRemoveAnimation

                    PropertyAction {
                        target: rootDelegate
                        property: "ListView.delayRemove"
                        value: true
                    }
                    NumberAnimation {
                        target: rootDelegate
                        property: "height"
                        to: 0
                        easing.type: Easing.InOutQuad
                    }
                    PropertyAction {
                        target: rootDelegate;
                        property: "ListView.delayRemove";
                        value: false
                    }
                }

                swipe.left: Rectangle {
                    color: rootDelegate.swipe.complete && rootDelegate.pressed ? "#333" : "#444"
                    anchors.fill: parent

                    Label {
                        objectName: "label"
                        text: "Remove"
                        color: "white"
                        anchors.centerIn: parent
                    }
                }
            }
        }
    }

    function test_removableDelegates() {
        var listView = removableDelegatesComponent.createObject(testCase);
        verify(listView);
        compare(listView.count, 3);

        // Expose the remove button.
        var firstItem = listView.itemAt(0, 0);
        mousePress(listView, firstItem.width / 2, firstItem.height / 2);
        verify(firstItem.pressed);
        compare(firstItem.swipe.position, 0.0);
        verify(!firstItem.swipe.complete);

        mouseMove(listView, firstItem.width * 1.1, firstItem.height / 2);
        verify(firstItem.pressed);
        compare(firstItem.swipe.position, 0.6);
        verify(!firstItem.swipe.complete);

        mouseRelease(listView, firstItem.width / 2, firstItem.height / 2);
        verify(!firstItem.pressed);
        compare(firstItem.swipe.position, 1.0);
        verify(firstItem.swipe.complete);
        compare(listView.count, 3);

        // Wait for it to settle down.
        tryCompare(firstItem.contentItem, "x", firstItem.leftPadding + firstItem.width);

        // Click the button to remove the item.
        var contentItemX = firstItem.contentItem.x;
        mouseClick(listView, firstItem.width / 2, firstItem.height / 2);
        tryCompare(firstItem.removeAnimation, "running", true);
        // There was a bug where the resizeContent() would be called because the height
        // of the control was changing due to the animation. contentItem would then
        // change x position and hence be visible when it shouldn't be.
        verify(firstItem.removeAnimation.running);
        while (1) {
            wait(10)
            if (firstItem && firstItem.removeAnimation && firstItem.removeAnimation.running)
                compare(firstItem.contentItem.x, contentItemX);
            else
                break;
        }
        compare(listView.count, 2);

        listView.destroy();
    }

    Component {
        id: leadingTrailingXComponent
        SwipeDelegate {
            id: delegate
            width: 150
            text: "SwipeDelegate"

            swipe.left: Rectangle {
                x: delegate.background.x - width
                width: delegate.width
                height: delegate.height
                color: "green"
            }

            swipe.right: Rectangle {
                x: delegate.background.x + delegate.background.width
                width: delegate.width
                height: delegate.height
                color: "red"
            }
        }
    }

    Component {
        id: leadingTrailingAnchorsComponent
        SwipeDelegate {
            id: delegate
            width: 150
            text: "SwipeDelegate"

            swipe.left: Rectangle {
                anchors.right: delegate.background.left
                width: delegate.width
                height: delegate.height
                color: "green"
            }

            swipe.right: Rectangle {
                anchors.left: delegate.background.right
                width: delegate.width
                height: delegate.height
                color: "red"
            }
        }
    }

    function test_leadingTrailing_data() {
        return [
            { tag: "x", component: leadingTrailingXComponent },
            { tag: "anchors", component: leadingTrailingAnchorsComponent },
        ];
    }

    function test_leadingTrailing(data) {
        var control = data.component.createObject(testCase);
        verify(control);

        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton);
        mouseMove(control, control.width, control.height / 2, Qt.LeftButton);
        verify(control.swipe.leftItem);
        compare(control.swipe.leftItem.x, -control.width / 2);
        mouseRelease(control, control.width / 2, control.height / 2, Qt.LeftButton);

        control.destroy();
    }

    function test_minMaxPosition() {
        var control = leadingTrailingXComponent.createObject(testCase);
        verify(control);

        // Should be limited within the range -1.0 to 1.0.
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton);
        mouseMove(control, control.width * 1.5, control.height / 2, Qt.LeftButton);
        compare(control.swipe.position, 1.0);
        mouseMove(control, control.width * 1.6, control.height / 2, Qt.LeftButton);
        compare(control.swipe.position, 1.0);
        mouseMove(control, control.width * -1.6, control.height / 2, Qt.LeftButton);
        compare(control.swipe.position, -1.0);
        mouseRelease(control, control.width / 2, control.height / 2, Qt.LeftButton);

        control.destroy();
    }

    Component {
        id: emptySwipeDelegateComponent

        SwipeDelegate {
            text: "SwipeDelegate"
            width: 150
        }
    }

    Component {
        id: smallLeftComponent

        Rectangle {
            width: 80
            height: 40
            color: "green"
        }
    }

    // swipe.position should be scaled to the width of the relevant delegate,
    // and it shouldn't be possible to drag past the delegate (so that content behind the control is visible).
    function test_delegateWidth() {
        var control = emptySwipeDelegateComponent.createObject(testCase);
        verify(control);

        control.swipe.left = smallLeftComponent;

        // Ensure that the position is scaled to the width of the currently visible delegate.
        var overDragDistance = dragDistance * 1.1;
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton);
        mouseMove(control, control.width / 2 + overDragDistance, control.height / 2, Qt.LeftButton);
        verify(control.swipe.leftItem);
        compare(control.swipe.position, overDragDistance / control.swipe.leftItem.width);

        mouseMove(control, control.width / 2 + control.swipe.leftItem.width, control.height / 2, Qt.LeftButton);
        compare(control.swipe.position, 1.0);

        // Ensure that it's not possible to drag past the (left) delegate.
        mouseMove(control, control.width / 2 + control.swipe.leftItem.width + 1, control.height / 2, Qt.LeftButton);
        compare(control.swipe.position, 1.0);

        // Now release over the right side; the position should be 1.0 and the background
        // should be "anchored" to the right side of the left delegate item.
        mouseMove(control, control.width / 2 + control.swipe.leftItem.width, control.height / 2, Qt.LeftButton);
        mouseRelease(control, control.width / 2 + control.swipe.leftItem.width, control.height / 2, Qt.LeftButton);
        compare(control.swipe.position, 1.0);
        tryCompare(control.background, "x", control.swipe.leftItem.width, 1000);

        control.destroy();
    }

    SignalSpy {
        id: leftVisibleSpy
        signalName: "visibleChanged"
    }

    SignalSpy {
        id: rightVisibleSpy
        signalName: "visibleChanged"
    }

    function test_positionAfterSwipeCompleted() {
        var control = swipeDelegateComponent.createObject(testCase);
        verify(control);

        // Ensure that both delegates are constructed.
        mousePress(control, 0, control.height / 2, Qt.LeftButton);
        mouseMove(control, control.width * 1.1, control.height / 2, Qt.LeftButton);
        verify(control.swipe.leftItem);
        mouseMove(control, control.width * -0.1, control.height / 2, Qt.LeftButton);
        verify(control.swipe.rightItem);

        // Expose the left delegate.
        mouseMove(control, control.swipe.leftItem.width, control.height / 2, Qt.LeftButton);
        mouseRelease(control, control.swipe.leftItem.width, control.height / 2);
        verify(control.swipe.complete);
        compare(control.swipe.position, 1.0);

        leftVisibleSpy.target = control.swipe.leftItem;
        rightVisibleSpy.target = control.swipe.rightItem;

        // Swipe from right to left without exposing the right item,
        // and make sure that the right item never becomes visible
        // (and hence that the left item never loses visibility).
        mousePress(control, control.swipe.leftItem.width, control.height / 2, Qt.LeftButton);
        compare(leftVisibleSpy.count, 0);
        compare(rightVisibleSpy.count, 0);
        var newX = control.swipe.leftItem.width - dragDistance * 1.1;
        mouseMove(control, newX, control.height / 2, Qt.LeftButton, Qt.LeftButton);
        compare(leftVisibleSpy.count, 0);
        compare(rightVisibleSpy.count, 0);
        compare(control.swipe.position, newX / control.swipe.leftItem.width);

        mouseMove(control, 0, control.height / 2, Qt.LeftButton);
        compare(control.swipe.position, 0);

        // Test swiping over a distance that is greater than the width of the left item.
        mouseMove(control, -1, control.height / 2, Qt.LeftButton);
        verify(control.swipe.rightItem);
        compare(control.swipe.position, -1 / control.swipe.rightItem.width);

        // Now go back to 1.0.
        mouseMove(control, control.swipe.leftItem.width, control.height / 2, Qt.LeftButton);
        compare(control.swipe.position, 1.0);
        tryCompare(control.background, "x", control.swipe.leftItem.width, 1000);
        mouseRelease(control, control.swipe.leftItem.width, control.height / 2, Qt.LeftButton);

        control.destroy();
    }

    // TODO: this somehow results in the behind item having a negative width
//    Component {
//        id: behindSwipeDelegateComponent
//        SwipeDelegate {
//            anchors.centerIn: parent
//            swipe.behind: Rectangle {
//                onXChanged: print("x changed", x)
//                anchors.left: {
//                    print("anchors.left expression", swipe.position)
//                    swipe.position < 0 ? parent.background.right : undefined
//                }
//                anchors.right: {
//                    print("anchors.right expression", swipe.position)
//                    swipe.position > 0 ? parent.background.left : undefined
//                }
//                width: parent.width
//                height: parent.height
//                color: "green"
//            }
//            swipe.left: null
//            swipe.right: null
//            Rectangle {
//                anchors.fill: parent
//                color: "transparent"
//                border.color: "darkorange"
//            }
//        }
//    }

    Component {
        id: behindSwipeDelegateComponent
        SwipeDelegate {
            text: "SwipeDelegate"
            width: 150
            anchors.centerIn: parent
            swipe.behind: Rectangle {
                x: swipe.position < 0 ? parent.background.x + parent.background.width
                    : (swipe.position > 0 ? parent.background.x - width : 0)
                width: parent.width
                height: parent.height
                color: "green"
            }
            swipe.left: null
            swipe.right: null
        }
    }

    function test_leadingTrailingBehindItem() {
        var control = behindSwipeDelegateComponent.createObject(testCase);
        verify(control);

        swipe(control, 0.0, 1.0);
        verify(control.swipe.behindItem.visible);
        compare(control.swipe.behindItem.x, control.background.x - control.background.width);

        swipe(control, 1.0, -1.0);
        verify(control.swipe.behindItem.visible);
        compare(control.swipe.behindItem.x, control.background.x + control.background.width);

        swipe(control, -1.0, 1.0);
        verify(control.swipe.behindItem.visible);
        compare(control.swipe.behindItem.x, control.background.x - control.background.width);

        // Should be possible to "wrap" with a behind delegate specified.
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton);
        mouseMove(control, control.width / 2 + control.swipe.behindItem.width * 0.8, control.height / 2, Qt.LeftButton);
        compare(control.swipe.position, -0.2);
        mouseRelease(control, control.width / 2 + control.swipe.behindItem.width * 0.8, control.height / 2, Qt.LeftButton);
        compare(control.swipe.position, 0.0);

        // Try wrapping the other way.
        swipe(control, 0.0, -1.0);
        verify(control.swipe.behindItem.visible);
        compare(control.swipe.behindItem.x, control.background.x + control.background.width);

        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton);
        mouseMove(control, control.width / 2 - control.swipe.behindItem.width * 0.8, control.height / 2, Qt.LeftButton);
        compare(control.swipe.position, 0.2);
        mouseRelease(control, control.width / 2 - control.swipe.behindItem.width * 0.8, control.height / 2, Qt.LeftButton);
        compare(control.swipe.position, 0.0);

        control.destroy();
    }
}
