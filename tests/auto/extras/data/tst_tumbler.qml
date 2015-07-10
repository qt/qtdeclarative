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
import QtQuick.Extras 2.0

TestCase {
    id: testCase
    width: 200
    height: 200
    visible: true
    when: windowShown
    name: "Tumbler"

    property var tumbler: null

    function init() {
        tumbler = Qt.createQmlObject("import QtQuick.Extras 2.0; Tumbler { }", testCase, "");
        verify(tumbler, "Tumbler: failed to create an instance");
    }

    function cleanup() {
        tumbler.destroy();
    }

    function tumblerXCenter() {
        return tumbler.leftPadding + tumbler.width / 2;
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
        var keyClickDelay = 100;

        // Navigate upwards through entire wheel.
        for (var j = 0; j < tumbler.count - 1; ++j) {
            keyClick(Qt.Key_Up, Qt.NoModifier, keyClickDelay);
            tryCompare(tumbler.contentItem, "offset", j + 1);
            compare(tumbler.currentIndex, tumbler.count - 1 - j);
        }

        keyClick(Qt.Key_Up, Qt.NoModifier, keyClickDelay);
        tryCompare(tumbler.contentItem, "offset", 0);
        compare(tumbler.currentIndex, 0);

        // Navigate downwards through entire wheel.
        for (j = 0; j < tumbler.count - 1; ++j) {
            keyClick(Qt.Key_Down, Qt.NoModifier, keyClickDelay);
            tryCompare(tumbler.contentItem, "offset", tumbler.count - 1 - j);
            compare(tumbler.currentIndex, j + 1);
        }

        keyClick(Qt.Key_Down, Qt.NoModifier, keyClickDelay);
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

    function test_resizeAfterFlicking() {
        // Test QTBUG-40367 (which is actually invalid because it was my fault :)).
        tumbler.model = 100;

        // Flick in some direction.
        var pos = Qt.point(tumblerXCenter(), tumbler.topPadding);
        mouseDrag(tumbler, pos.x, pos.y, 0, tumbler.height - tumbler.bottomPadding,
            Qt.LeftButton, Qt.NoModifier, 400);
        tryCompare(tumbler.contentItem, "offset", 3.0);

        tumbler.height += 100;
        compare(tumbler.contentItem.delegateHeight,
            (tumbler.height - tumbler.topPadding - tumbler.bottomPadding) / tumbler.visibleItemCount);
        waitForRendering(tumbler);
        pos = itemCenterPos(1);
        var ninetyEighthItem = tumbler.contentItem.itemAt(pos.x, pos.y);
        verify(ninetyEighthItem);
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
            { index: 0, offset: 0, expectedDisplacement: 0 },
            { index: 1, offset: 0, expectedDisplacement: -1 },
            { index: 5, offset: 0, expectedDisplacement: 1 },
            // When we start to move the first item down, the second item above it starts to become current.
            { index: 0, offset: 0.25, expectedDisplacement: -0.25 },
            { index: 1, offset: 0.25, expectedDisplacement: -1.25 },
            { index: 5, offset: 0.25, expectedDisplacement: 0.75 },
            { index: 0, offset: 0.5, expectedDisplacement: -0.5 },
            { index: 1, offset: 0.5, expectedDisplacement: -1.5 },
            { index: 5, offset: 0.5, expectedDisplacement: 0.5 },
            // By this stage, the delegate at index 1 is destroyed, so we can't test its displacement.
            { index: 0, offset: 0.75, expectedDisplacement: -0.75 },
            { index: 5, offset: 0.75, expectedDisplacement: 0.25 },
            { index: 0, offset: 4.75, expectedDisplacement: 1.25 },
            { index: 1, offset: 4.75, expectedDisplacement: 0.25 },
            { index: 0, offset: 4.5, expectedDisplacement: 1.5 },
            { index: 1, offset: 4.5, expectedDisplacement: 0.5 },
            { index: 0, offset: 4.25, expectedDisplacement: 1.75 },
            { index: 1, offset: 4.25, expectedDisplacement: 0.75 }
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
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter

        property real displacement: AbstractTumbler.displacement
    }

    function test_displacement(data) {
        // TODO: test setting these in the opposite order (delegate after model
        // doesn't seem to cause a change in delegates in PathView)
        tumbler.delegate = displacementDelegate;
        tumbler.model = 6;
        compare(tumbler.count, 6);

        var delegate = findChild(tumbler.contentItem, "delegate" + data.index);
        verify(delegate);

        tumbler.contentItem.offset = data.offset;
        compare(delegate.displacement, data.expectedDisplacement);

        // test displacement after adding and removing items
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
        property real displacement: AbstractTumbler.displacement
    }

    property Component noParentDelegateComponent: Item {
        property real displacement: AbstractTumbler.displacement
    }

    property Component listViewComponent: ListView {}
    property Component simpleDisplacementDelegate: Text {
        property real displacement: AbstractTumbler.displacement
        property int index: -1
    }

    function test_attachedProperties() {
        // TODO: crashes somewhere in QML's guts
//        tumbler.model = 5;
//        tumbler.delegate = wrongDelegateTypeComponent;
//        ignoreWarning("Attached properties of Tumbler must be accessed from within a delegate item");
//        // Cause displacement to be changed. The warning isn't triggered if we don't do this.
//        tumbler.contentItem.offset += 1;

        ignoreWarning("Attached properties of Tumbler must be accessed from within a delegate item that has a parent");
        noParentDelegateComponent.createObject(null);

        ignoreWarning("Attempting to access attached property on item without an \"index\" property");
        var object = noParentDelegateComponent.createObject(testCase);
        object.destroy();

        var listView = listViewComponent.createObject(testCase);
        ignoreWarning("contentItems other than PathView are not currently supported");
        object = simpleDisplacementDelegate.createObject(listView);
        object.destroy();
        listView.destroy();
    }
}
