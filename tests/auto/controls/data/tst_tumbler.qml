/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

import QtQuick 2.2
import QtTest 1.0
import QtQuick.Controls 2.0

TestCase {
    id: testCase
    width: 200
    height: 200
    visible: true
    when: windowShown
    name: "Tumbler"

    property var tumbler: null
    readonly property real implicitTumblerWidth: 60
    readonly property real implicitTumblerHeight: 200
    readonly property real defaultImplicitDelegateHeight: implicitTumblerHeight / 3
    readonly property real defaultListViewTumblerOffset: -defaultImplicitDelegateHeight

    function init() {
        tumbler = Qt.createQmlObject("import QtQuick.Controls 2.0; Tumbler { }", testCase, "");
        verify(tumbler, "Tumbler: failed to create an instance");
        compare(tumbler.contentItem.parent, tumbler);
    }

    function cleanup() {
        tumbler.destroy();
    }

    function tumblerXCenter() {
        return tumbler.leftPadding + tumbler.width / 2;
    }

    function tumblerYCenter() {
        return tumbler.topPadding + tumbler.height / 2;
    }

    // visualItemIndex is from 0 to the amount of visible items.
    function itemCenterPos(visualItemIndex) {
        var halfDelegateHeight = tumbler.contentItem.delegateHeight / 2;
        var yCenter = tumbler.y + tumbler.topPadding + halfDelegateHeight
            + (tumbler.contentItem.delegateHeight * visualItemIndex);
        return Qt.point(tumblerXCenter(), yCenter);
    }

    function checkItemSizes() {
        var contentChildren = tumbler.contentItem.hasOwnProperty("contentItem")
            ? tumbler.contentItem.contentItem.children : tumbler.contentItem.children;
        verify(contentChildren.length >= tumbler.count);
        for (var i = 0; i < contentChildren.length; ++i) {
            compare(contentChildren[i].width, tumbler.width);
            compare(contentChildren[i].height, tumbler.contentItem.delegateHeight);
        }
    }

    function tst_dynamicContentItemChange() {
        // test that currentIndex is maintained between contentItem changes...
    }

    function test_currentIndex() {
        tumbler.model = 5;

        compare(tumbler.currentIndex, 0);
        waitForRendering(tumbler);

        // Set it through user interaction.
        var pos = Qt.point(tumblerXCenter(), tumbler.height / 2);
        mouseDrag(tumbler, pos.x, pos.y, 0, -tumbler.contentItem.delegateHeight / 2, Qt.LeftButton, Qt.NoModifier, 200);
        compare(tumbler.currentIndex, 1);
        compare(tumbler.contentItem.currentIndex, 1);

        // Set it manually.
        tumbler.currentIndex = 2;
        tryCompare(tumbler, "currentIndex", 2);
        compare(tumbler.contentItem.currentIndex, 2);

        // PathView has 0 as its currentIndex in this case for some reason.
        tumbler.model = null;
        tryCompare(tumbler, "currentIndex", 0);

        tumbler.model = ["A", "B", "C"];
        tryCompare(tumbler, "currentIndex", 0);
    }

    function test_keyboardNavigation() {
        tumbler.model = 5;
        tumbler.forceActiveFocus();
        tumbler.contentItem.highlightMoveDuration = 0;

        // Navigate upwards through entire wheel.
        for (var j = 0; j < tumbler.count - 1; ++j) {
            keyClick(Qt.Key_Up, Qt.NoModifier);
            tryCompare(tumbler.contentItem, "offset", j + 1);
            compare(tumbler.currentIndex, tumbler.count - 1 - j);
        }

        keyClick(Qt.Key_Up, Qt.NoModifier);
        tryCompare(tumbler.contentItem, "offset", 0);
        compare(tumbler.currentIndex, 0);

        // Navigate downwards through entire wheel.
        for (j = 0; j < tumbler.count - 1; ++j) {
            keyClick(Qt.Key_Down, Qt.NoModifier);
            tryCompare(tumbler.contentItem, "offset", tumbler.count - 1 - j);
            compare(tumbler.currentIndex, j + 1);
        }

        keyClick(Qt.Key_Down, Qt.NoModifier);
        tryCompare(tumbler.contentItem, "offset", 0);
        compare(tumbler.currentIndex, 0);
    }

    function test_itemsCorrectlyPositioned() {
        tumbler.model = 4;
        tumbler.height = 120;
        compare(tumbler.contentItem.delegateHeight, 40);
        checkItemSizes();

        wait(tumbler.contentItem.highlightMoveDuration);
        var firstItemCenterPos = itemCenterPos(1);
        var firstItem = tumbler.contentItem.itemAt(firstItemCenterPos.x, firstItemCenterPos.y);
        var actualPos = testCase.mapFromItem(firstItem, 0, 0);
        compare(actualPos.x, tumbler.leftPadding);
        compare(actualPos.y, tumbler.topPadding + 40);

        tumbler.forceActiveFocus();
        keyClick(Qt.Key_Down);
        tryCompare(tumbler.contentItem, "offset", 3.0);
        firstItemCenterPos = itemCenterPos(0);
        firstItem = tumbler.contentItem.itemAt(firstItemCenterPos.x, firstItemCenterPos.y);
        verify(firstItem);
        // Test QTBUG-40298.
        actualPos = testCase.mapFromItem(firstItem, 0, 0);
        compare(actualPos.x, tumbler.leftPadding);
        compare(actualPos.y, tumbler.topPadding);

        var secondItemCenterPos = itemCenterPos(1);
        var secondItem = tumbler.contentItem.itemAt(secondItemCenterPos.x, secondItemCenterPos.y);
        verify(secondItem);
        verify(firstItem.y < secondItem.y);

        var thirdItemCenterPos = itemCenterPos(2);
        var thirdItem = tumbler.contentItem.itemAt(thirdItemCenterPos.x, thirdItemCenterPos.y);
        verify(thirdItem);
        verify(firstItem.y < thirdItem.y);
        verify(secondItem.y < thirdItem.y);
    }

    function test_focusPastTumbler() {
        var mouseArea = Qt.createQmlObject(
            "import QtQuick 2.2; TextInput { activeFocusOnTab: true; width: 50; height: 50 }", testCase, "");

        tumbler.forceActiveFocus();
        verify(tumbler.activeFocus);

        keyClick(Qt.Key_Tab);
        verify(!tumbler.activeFocus);
        verify(mouseArea.activeFocus);

        mouseArea.destroy();
    }

    function test_datePicker() {
        tumbler.destroy();

        var component = Qt.createComponent("TumblerDatePicker.qml");
        compare(component.status, Component.Ready, component.errorString());
        tumbler = component.createObject(testCase);
        // Should not be any warnings.

        compare(tumbler.dayTumbler.currentIndex, 0);
        compare(tumbler.dayTumbler.count, 31);
        compare(tumbler.monthTumbler.currentIndex, 0);
        compare(tumbler.monthTumbler.count, 12);
        compare(tumbler.yearTumbler.currentIndex, 0);
        compare(tumbler.yearTumbler.count, 100);

        verify(tumbler.dayTumbler.contentItem.children.length >= tumbler.dayTumbler.visibleItemCount);
        verify(tumbler.monthTumbler.contentItem.children.length >= tumbler.monthTumbler.visibleItemCount);
        // TODO: do this properly somehow
        wait(100);
        verify(tumbler.yearTumbler.contentItem.children.length >= tumbler.yearTumbler.visibleItemCount);

        // March.
        tumbler.monthTumbler.currentIndex = 2;
        tryCompare(tumbler.monthTumbler, "currentIndex", 2);

        // 30th of March.
        tumbler.dayTumbler.currentIndex = 29;
        tryCompare(tumbler.dayTumbler, "currentIndex", 29);

        // February.
        tumbler.monthTumbler.currentIndex = 1;
        tryCompare(tumbler.monthTumbler, "currentIndex", 1);
        tryCompare(tumbler.dayTumbler, "currentIndex", 27);
    }

    function test_displacement_data() {
        var data = [
            // At 0 offset, the first item is current.
            { count: 6, index: 0, offset: 0, expectedDisplacement: 0 },
            { count: 6, index: 1, offset: 0, expectedDisplacement: -1 },
            { count: 6, index: 5, offset: 0, expectedDisplacement: 1 },
            // When we start to move the first item down, the second item above it starts to become current.
            { count: 6, index: 0, offset: 0.25, expectedDisplacement: -0.25 },
            { count: 6, index: 1, offset: 0.25, expectedDisplacement: -1.25 },
            { count: 6, index: 5, offset: 0.25, expectedDisplacement: 0.75 },
            { count: 6, index: 0, offset: 0.5, expectedDisplacement: -0.5 },
            { count: 6, index: 1, offset: 0.5, expectedDisplacement: -1.5 },
            { count: 6, index: 5, offset: 0.5, expectedDisplacement: 0.5 },
            // By this stage, the delegate at index 1 is destroyed, so we can't test its displacement.
            { count: 6, index: 0, offset: 0.75, expectedDisplacement: -0.75 },
            { count: 6, index: 5, offset: 0.75, expectedDisplacement: 0.25 },
            { count: 6, index: 0, offset: 4.75, expectedDisplacement: 1.25 },
            { count: 6, index: 1, offset: 4.75, expectedDisplacement: 0.25 },
            { count: 6, index: 0, offset: 4.5, expectedDisplacement: 1.5 },
            { count: 6, index: 1, offset: 4.5, expectedDisplacement: 0.5 },
            { count: 6, index: 0, offset: 4.25, expectedDisplacement: 1.75 },
            { count: 6, index: 1, offset: 4.25, expectedDisplacement: 0.75 },
            // count == visibleItemCount
            { count: 3, index: 0, offset: 0, expectedDisplacement: 0 },
            { count: 3, index: 1, offset: 0, expectedDisplacement: -1 },
            { count: 3, index: 2, offset: 0, expectedDisplacement: 1 },
            // count < visibleItemCount
            { count: 2, index: 0, offset: 0, expectedDisplacement: 0 },
            { count: 2, index: 1, offset: 0, expectedDisplacement: 1 },
            // count == 1
            { count: 1, index: 0, offset: 0, expectedDisplacement: 0 }
        ];
        for (var i = 0; i < data.length; ++i) {
            var row = data[i];
            row.tag = "delegate" + row.index + " offset=" + row.offset + " expectedDisplacement=" + row.expectedDisplacement;
        }
        return data;
    }

    property Component displacementDelegate: Text {
        objectName: "delegate" + index
        text: modelData
        opacity: 0.2 + Math.max(0, 1 - Math.abs(Tumbler.displacement)) * 0.8
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter

        Text {
            text: parent.displacement.toFixed(2)
            anchors.right: parent.right
        }

        property real displacement: Tumbler.displacement
    }

    function test_displacement(data) {
        // TODO: test setting these in the opposite order (delegate after model
        // doesn't seem to cause a change in delegates in PathView)
        tumbler.delegate = displacementDelegate;
        tumbler.model = data.count;
        compare(tumbler.count, data.count);

        var delegate = findChild(tumbler.contentItem, "delegate" + data.index);
        verify(delegate);

        tumbler.contentItem.offset = data.offset;
        compare(delegate.displacement, data.expectedDisplacement);

        // test displacement after adding and removing items
    }

    Component {
        id: listViewTumblerComponent
        //! [contentItem]
        Tumbler {
            id: listViewTumbler

            contentItem: ListView {
                anchors.fill: parent
                model: listViewTumbler.model
                delegate: listViewTumbler.delegate

                snapMode: ListView.SnapToItem
                highlightRangeMode: ListView.StrictlyEnforceRange
                preferredHighlightBegin: height / 2 - (height / listViewTumbler.visibleItemCount / 2)
                preferredHighlightEnd: height / 2  + (height / listViewTumbler.visibleItemCount / 2)
                clip: true
            }
        }
        //! [contentItem]
    }

    function test_displacementListView_data() {
        var offset = defaultListViewTumblerOffset;

        var data = [
            // At 0 contentY, the first item is current.
            { contentY: offset, expectedDisplacements: [
                { index: 0, displacement: 0 },
                { index: 1, displacement: -1 },
                { index: 2, displacement: -2 } ]
            },
            // When we start to move the first item down, the second item above it starts to become current.
            { contentY: offset + defaultImplicitDelegateHeight * 0.25, expectedDisplacements: [
                { index: 0, displacement: 0.25 },
                { index: 1, displacement: -0.75 },
                { index: 2, displacement: -1.75 } ]
            },
            { contentY: offset + defaultImplicitDelegateHeight * 0.5, expectedDisplacements: [
                { index: 0, displacement: 0.5 },
                { index: 1, displacement: -0.5 },
                { index: 2, displacement: -1.5 } ]
            },
            { contentY: offset + defaultImplicitDelegateHeight * 0.75, expectedDisplacements: [
                { index: 0, displacement: 0.75 },
                { index: 1, displacement: -0.25 } ]
            },
            { contentY: offset + defaultImplicitDelegateHeight * 3.5, expectedDisplacements: [
                { index: 3, displacement: 0.5 },
                { index: 4, displacement: -0.5 } ]
            }
        ];
        for (var i = 0; i < data.length; ++i) {
            var row = data[i];
            row.tag = "contentY=" + row.contentY;
        }
        return data;
    }

    function test_displacementListView(data) {
        tumbler.destroy();
        // Sanity check that they're aren't any children at this stage.
        tryCompare(testCase.children, "length", 0);

        tumbler = listViewTumblerComponent.createObject(testCase);
        verify(tumbler);

        tumbler.delegate = displacementDelegate;
        tumbler.model = 5;
        compare(tumbler.count, 5);
        // Ensure assumptions about the tumbler used in our data() function are correct.
        compare(tumbler.contentItem.contentY, -defaultImplicitDelegateHeight);
        var delegateCount = 0;
        var listView = tumbler.contentItem;
        var listViewContentItem = tumbler.contentItem.contentItem;

        // We use the mouse instead of setting contentY directly, otherwise the
        // items snap back into place. This doesn't seem to be an issue for
        // PathView for some reason.
        //
        // I tried lots of things to get this test to work with small changes
        // in ListView's contentY (to match the tests for a PathView-based Tumbler), but they didn't work:
        //
        // - Pressing once and then directly moving the mouse to the correct location
        // - Pressing once and interpolating the mouse position to the correct location
        // - Pressing once and doing some dragging up and down to trigger the
        //   overThreshold of QQuickFlickable
        //
        // Even after the last item above, QQuickFlickable wouldn't consider it a drag.
        // It seems that overThreshold is set too late, and because the drag distance is quite small
        // to begin with, nothing changes (the displacement was always very close to 0 in the end).

        // Ensure that we at least cover the distance required to reach the desired contentY.
        var distanceToReachContentY = data.contentY - defaultListViewTumblerOffset;
        var distance = Math.abs(distanceToReachContentY) + tumbler.height / 2;
        // If distanceToReachContentY is 0, we're testing 0 displacement, so we don't need to do anything.
        if (distanceToReachContentY != 0) {
            mousePress(tumbler, tumblerXCenter(), tumblerYCenter());

            var dragDirection = distanceToReachContentY > 0 ? -1 : 1;
            for (var i = 0; i < distance && Math.floor(listView.contentY) !== Math.floor(data.contentY); ++i) {
                mouseMove(tumbler, tumblerXCenter(), tumblerYCenter() + i * dragDirection);
            }
        }

        for (var i = 0; i < data.expectedDisplacements.length; ++i) {
            var delegate = findChild(listViewContentItem, "delegate" + data.expectedDisplacements[i].index);
            verify(delegate);
            compare(delegate.height, defaultImplicitDelegateHeight);
            // Due to the way we must perform this test, we can't expect high precision.
            var expectedDisplacement = data.expectedDisplacements[i].displacement;
            fuzzyCompare(delegate.displacement, expectedDisplacement, 0.1,
                "Delegate of ListView-based Tumbler at index " + data.expectedDisplacements[i].index
                    + " has displacement of " + delegate.displacement + " when it should be " + expectedDisplacement);
        }

        if (distanceToReachContentY != 0)
            mouseRelease(tumbler, tumblerXCenter(), itemCenterPos(1) + (data.contentY - defaultListViewTumblerOffset), Qt.LeftButton);
    }

    function test_listViewFlickAboveBounds_data() {
        // Tests that flicking above the bounds when already at the top of the
        // tumbler doesn't result in an incorrect displacement.
        var data = [];
        // Less than two items doesn't make sense. The default visibleItemCount
        // is 3, so we test a bit more than double that.
        for (var i = 2; i <= 7; ++i) {
            data.push({ tag: i + " items", model: i });
        }
        return data;
    }

    function test_listViewFlickAboveBounds(data) {
        tumbler.destroy();

        tumbler = listViewTumblerComponent.createObject(testCase);
        verify(tumbler);

        tumbler.delegate = displacementDelegate;
        tumbler.model = data.model;

        mousePress(tumbler, tumblerXCenter(), tumblerYCenter());

        // Ensure it's stationary.
        var listView = tumbler.contentItem;
        compare(listView.contentY, defaultListViewTumblerOffset);

        // We could just move up until the contentY changed, but this is safer.
        var distance = tumbler.height;
        var changed = false;

        for (var i = 0; i < distance && !changed; ++i) {
            mouseMove(tumbler, tumblerXCenter(), tumblerYCenter() + i, 10);

            // Don't test until the contentY has actually changed.
            if (Math.abs(listView.contentY) - listView.preferredHighlightBegin > 0.01) {

                for (var delegateIndex = 0; delegateIndex < Math.min(tumbler.count, tumbler.visibleItemCount); ++delegateIndex) {
                    var delegate = findChild(listView.contentItem, "delegate" + delegateIndex);
                    verify(delegate);

                    verify(delegate.displacement <= -delegateIndex, "Delegate at index " + delegateIndex + " has a displacement of "
                        + delegate.displacement + " when it should be less than or equal to " + -delegateIndex);
                    verify(delegate.displacement > -delegateIndex - 0.1, "Delegate at index 0 has a displacement of "
                        + delegate.displacement + " when it should be greater than ~ " + -delegateIndex - 0.1);
                }

                changed = true;
            }
        }

        // Sanity check that something was actually tested.
        verify(changed);

        mouseRelease(tumbler, tumblerXCenter(), tumbler.topPadding);
    }

    property Component objectNameDelegate: Text {
        objectName: "delegate" + index
        text: modelData
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    function test_visibleItemCount_data() {
        var data = [
            // e.g. {0: 2} = {delegate index: y pos / delegate height}
            // Skip item at index 3, because it's out of view.
            { model: 6, visibleItemCount: 5, expectedYPositions: {0: 2, 1: 3, 2: 4, 4: 0} },
            { model: 5, visibleItemCount: 3, expectedYPositions: {0: 1, 1: 2, 4: 0} },
            // Takes up the whole view.
            { model: 2, visibleItemCount: 1, expectedYPositions: {0: 0} },
        ];

        for (var i = 0; i < data.length; ++i) {
            data[i].tag = "items=" + data[i].model + ", visibleItemCount=" + data[i].visibleItemCount;
        }
        return data;
    }

    function test_visibleItemCount(data) {
        tumbler.delegate = objectNameDelegate;
        tumbler.visibleItemCount = data.visibleItemCount;

        tumbler.model = data.model;
        compare(tumbler.count, data.model);

        for (var delegateIndex = 0; delegateIndex < data.visibleItemCount; ++delegateIndex) {
            if (data.expectedYPositions.hasOwnProperty(delegateIndex)) {
                var delegate = findChild(tumbler.contentItem, "delegate" + delegateIndex);
                verify(delegate, "Delegate found at index " + delegateIndex);
                var expectedYPos = data.expectedYPositions[delegateIndex] * tumbler.contentItem.delegateHeight;
                compare(delegate.mapToItem(tumbler.contentItem, 0, 0).y, expectedYPos);
            }
        }
    }

    property Component wrongDelegateTypeComponent: QtObject {
        property real displacement: Tumbler.displacement
    }

    property Component noParentDelegateComponent: Item {
        property real displacement: Tumbler.displacement
    }

    property Component gridViewComponent: GridView {}
    property Component simpleDisplacementDelegate: Text {
        property real displacement: Tumbler.displacement
        property int index: -1
    }

    function test_attachedProperties() {
        // TODO: crashes somewhere in QML's guts
//        tumbler.model = 5;
//        tumbler.delegate = wrongDelegateTypeComponent;
//        ignoreWarning("Attached properties of Tumbler must be accessed from within a delegate item");
//        // Cause displacement to be changed. The warning isn't triggered if we don't do this.
//        tumbler.contentItem.offset += 1;

        ignoreWarning("Tumbler: attached properties must be accessed from within a delegate item that has a parent");
        noParentDelegateComponent.createObject(null);

        ignoreWarning("Tumbler: attempting to access attached property on item without an \"index\" property");
        var object = noParentDelegateComponent.createObject(testCase);
        object.destroy();

        // Should not be any warnings from this, as ListView, for example, doesn't produce warnings for the same code.
        var gridView = gridViewComponent.createObject(testCase);
        object = simpleDisplacementDelegate.createObject(gridView);
        object.destroy();
        gridView.destroy();
    }

    property Component paddingDelegate: Text {
        objectName: "delegate" + index
        text: modelData
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter

        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.color: "red"
            border.width: 1
        }
    }

    function test_padding_data() {
        var data = [];

        data.push({ padding: 0 });
        data.push({ padding: 10 });
        data.push({ left: 10, top: 10 });
        data.push({ right: 10, bottom: 10 });

        for (var i = 0; i < data.length; ++i) {
            var tag = "";

            if (data[i].padding !== undefined)
                tag += "padding: " + data[i].padding + " ";
            if (data[i].left !== undefined)
                tag += "left: " + data[i].left + " ";
            if (data[i].right !== undefined)
                tag += "right: " + data[i].right + " ";
            if (data[i].top !== undefined)
                tag += "top: " + data[i].top + " ";
            if (data[i].bottom !== undefined)
                tag += "bottom: " + data[i].bottom + " ";
            tag = tag.slice(0, -1);

            data[i].tag = tag;
        }

        return data;
    }

    function test_padding(data) {
        tumbler.delegate = paddingDelegate;
        tumbler.model = 5;
        compare(tumbler.padding, 0);
        compare(tumbler.leftPadding, 0);
        compare(tumbler.rightPadding, 0);
        compare(tumbler.topPadding, 0);
        compare(tumbler.bottomPadding, 0);
        compare(tumbler.contentItem.x, 0);
        compare(tumbler.contentItem.y, 0);

        if (data.padding !== undefined)
            tumbler.padding = data.padding;
        if (data.left !== undefined)
            tumbler.leftPadding = data.left;
        if (data.right !== undefined)
            tumbler.rightPadding = data.right;
        if (data.top !== undefined)
            tumbler.topPadding = data.top;
        if (data.bottom !== undefined)
            tumbler.bottomPadding = data.bottom;

        compare(tumbler.availableWidth, implicitTumblerWidth - tumbler.leftPadding - tumbler.rightPadding);
        compare(tumbler.availableHeight, implicitTumblerHeight - tumbler.topPadding - tumbler.bottomPadding);
        compare(tumbler.contentItem.x, tumbler.leftPadding);
        compare(tumbler.contentItem.y, tumbler.topPadding);

        var pathView = tumbler.contentItem;
        var expectedDelegateHeight = tumbler.availableHeight / tumbler.visibleItemCount;
        var itemIndicesInVisualOrder = [4, 0, 1];
        for (var i = 0; i < itemIndicesInVisualOrder.length; ++i) {
            var delegate = findChild(pathView, "delegate" + itemIndicesInVisualOrder[i]);
            verify(delegate, "Couldn't find delegate at index " + itemIndicesInVisualOrder[i]
                + " (iteration " + i + " out of " + (pathView.children.length - 1) + ")");

            compare(delegate.width, tumbler.availableWidth);
            compare(delegate.height, expectedDelegateHeight);

            var expectedY = tumbler.topPadding + i * expectedDelegateHeight;
            var mappedPos = delegate.mapToItem(null, delegate.width / 2, 0);
            fuzzyCompare(mappedPos.y, expectedY, 0.5,
                "Tumbler's PathView delegate at index " + itemIndicesInVisualOrder[i]
                    + " should have a y pos of " + expectedY + ", but it's actually " + mappedPos.y.toFixed(20));

            var expectedX = tumbler.leftPadding;
            compare(delegate.mapToItem(null, 0, 0).x, expectedX,
                "Tumbler's PathView delegate at index " + itemIndicesInVisualOrder[i]
                    + " should have a x pos of " + expectedX + ", but it's actually " + mappedPos.x.toFixed(20));
        }

        // Force new items to be created, as there was a bug where the path was correct until this happened.
        compare(tumbler.contentItem.offset, 0);
        ++tumbler.currentIndex;
        tryCompare(tumbler.contentItem, "offset", 4, tumbler.contentItem.highlightMoveDuration * 2);
    }
}
