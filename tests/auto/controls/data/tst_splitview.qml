/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
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

import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Window 2.13
import QtTest 1.13
import Qt.labs.settings 1.0

TestCase {
    id: testCase
    width: 400
    height: 400
    visible: true
    when: windowShown
    name: "SplitView"

    function initTestCase() {
        // For the serialization tests.
        Qt.application.name = "qtquickcontrols2-splitview-auto-test"
        Qt.application.domain = "org.qt-project"
        Qt.application.organization = "Qt Project"
    }

    function findHandles(splitView) {
        var handles = []
        for (var i = 0; i < splitView.children.length; ++i) {
            var child = splitView.children[i]
            if (child.objectName.toLowerCase().indexOf("handle") !== -1)
                handles.push(child)
        }
        return handles
    }

    function compareSizes(control, expectedGeometries, context) {
        if (context === undefined)
            context = ""
        else
            context = " (" + context + ")"

        compare(control.count, Math.floor(expectedGeometries.length / 2) + 1,
            "Mismatch in actual vs expected number of split items" + context)

        var handles = findHandles(control)
        compare(handles.length, Math.floor(expectedGeometries.length / 2),
            "Mismatch in actual vs expected number of handle items" + context)

        for (var i = 0, splitItemIndex = 0, handleItemIndex = 0; i < expectedGeometries.length; ++i) {
            var item = null
            var itemType = ""
            var typeSpecificIndex = -1
            if (i % 2 == 0) {
                item = control.itemAt(splitItemIndex)
                itemType = "split item"
                typeSpecificIndex = splitItemIndex
                ++splitItemIndex
            } else {
                item = handles[handleItemIndex]
                itemType = "handle item"
                typeSpecificIndex = handleItemIndex
                ++handleItemIndex
            }

            verify(item, itemType + " at index " + typeSpecificIndex + " should not be null" + context)

            var expectedGeometry = expectedGeometries[i]
            if (expectedGeometry.hasOwnProperty("hidden")) {
                // It's geometry doesn't matter because it's hidden.
                verify(!item.visible, itemType + " at index " + typeSpecificIndex + " should be hidden" + context)
                continue
            }

            // Note that the indices mentioned here account for handles; they do not
            // match the indices reported by QQuickSplitView's logging categories.
            compare(item.x, expectedGeometry.x, "Mismatch in actual vs expected x value of "
                + itemType + " at index " + typeSpecificIndex + context)
            compare(item.y, expectedGeometry.y, "Mismatch in actual vs expected y value of "
                + itemType + " at index " + typeSpecificIndex + context)
            compare(item.width, expectedGeometry.width, "Mismatch in actual vs expected width value of "
                + itemType + " at index " + typeSpecificIndex + context)
            compare(item.height, expectedGeometry.height, "Mismatch in actual vs expected height value of "
                + itemType + " at index " + typeSpecificIndex + context)
        }
    }

    property real defaultHorizontalHandleWidth: 10
    property real defaultVerticalHandleHeight: 10


    Component {
        id: signalSpyComponent
        SignalSpy {}
    }

    Component {
        id: handleComponent
        Rectangle {
            objectName: "handle"
            implicitWidth: defaultHorizontalHandleWidth
            implicitHeight: defaultVerticalHandleHeight
            color: "#444"

            Text {
                objectName: "handleText_" + text
                text: parent.x + "," + parent.y + " " + parent.width + "x" + parent.height
                color: "white"
                anchors.centerIn: parent
                rotation: 90
            }
        }
    }

    Component {
        id: splitViewComponent

        SplitView {
            anchors.fill: parent
            handle: handleComponent
        }
    }

    Component {
        id: rectangleComponent

        Rectangle {}
    }

    function test_addItemsAfterCompletion() {
        var control = createTemporaryObject(splitViewComponent, testCase)
        verify(control)

        var item0 = rectangleComponent.createObject(control, { implicitWidth: 25, color: "salmon" })
        verify(item0)
        verify(isPolishScheduled(control))
        verify(waitForItemPolished(control))
        // The last item fills the width by default, and since there is only one item...
        compare(item0.x, 0)
        compare(item0.y, 0)
        compare(item0.width, testCase.width)
        compare(item0.height, testCase.height)

        var item1 = rectangleComponent.createObject(control, { implicitWidth: 25, color: "steelblue" })
        verify(item1)
        verify(isPolishScheduled(control))
        verify(waitForItemPolished(control))
        // Now that a second item has been added, the first item goes back to its preferred (implicit) width.
        compare(item0.x, 0)
        compare(item0.y, 0)
        compare(item0.width, item0.implicitWidth)
        compare(item0.height, testCase.height)
        var handles = findHandles(control)
        var handle0 = handles[0]
        compare(handle0.x, item0.implicitWidth)
        compare(handle0.y, 0)
        compare(handle0.width, defaultHorizontalHandleWidth)
        compare(handle0.height, testCase.height)
        compare(item1.x, item0.implicitWidth + defaultHorizontalHandleWidth)
        compare(item1.y, 0)
        compare(item1.width, testCase.width - item0.implicitWidth - defaultHorizontalHandleWidth)
        compare(item1.height, testCase.height)
    }

    function test_addItemsWithNoSizeAfterCompletion() {
        var control = createTemporaryObject(splitViewComponent, testCase)
        verify(control)

        var item0 = rectangleComponent.createObject(control, { color: "salmon" })
        verify(item0)
        verify(isPolishScheduled(control))
        verify(waitForItemPolished(control))
        compare(item0.x, 0)
        compare(item0.y, 0)
        compare(item0.width, testCase.width)
        compare(item0.height, testCase.height)

        var item1 = rectangleComponent.createObject(control, { color: "steelblue" })
        verify(item1)
        verify(isPolishScheduled(control))
        verify(waitForItemPolished(control))
        compare(item0.x, 0)
        compare(item0.y, 0)
        compare(item0.width, 0)
        compare(item0.height, testCase.height)
        var handles = findHandles(control)
        var handle0 = handles[0]
        compare(handle0.x, 0)
        compare(handle0.y, 0)
        compare(handle0.width, defaultHorizontalHandleWidth)
        compare(handle0.height, testCase.height)
        compare(item1.x, defaultHorizontalHandleWidth)
        compare(item1.y, 0)
        compare(item1.width, testCase.width - defaultHorizontalHandleWidth)
        compare(item1.height, testCase.height)
    }

    Component {
        id: threeZeroSizedItemsComponent

        SplitView {
            anchors.fill: parent
            handle: handleComponent

            Rectangle {
                objectName: "salmon"
                color: objectName
            }
            Rectangle {
                objectName: "navajowhite"
                color: objectName
            }
            Rectangle {
                objectName: "steelblue"
                color: objectName
            }
        }
    }

    function test_changeAttachedPropertiesAfterCompletion() {
        var control = createTemporaryObject(threeZeroSizedItemsComponent, testCase)
        verify(control)

        var item0 = control.itemAt(0)
        compare(item0.x, 0)
        compare(item0.y, 0)
        compare(item0.width, 0)
        compare(item0.height, testCase.height)

        var handles = findHandles(control)
        var handle0 = handles[0]
        compare(handle0.x, 0)
        compare(handle0.y, 0)
        compare(handle0.width, defaultHorizontalHandleWidth)
        compare(handle0.height, testCase.height)

        var item1 = control.itemAt(1)
        compare(item1.x, defaultHorizontalHandleWidth)
        compare(item1.y, 0)
        compare(item1.width, 0)
        compare(item1.height, testCase.height)

        var handle1 = handles[1]
        compare(handle1.x, defaultHorizontalHandleWidth)
        compare(handle1.y, 0)
        compare(handle1.width, defaultHorizontalHandleWidth)
        compare(handle1.height, testCase.height)

        var item2 = control.itemAt(2)
        compare(item2.x, defaultHorizontalHandleWidth * 2)
        compare(item2.y, 0)
        compare(item2.width, testCase.width - item2.x)
        compare(item2.height, testCase.height)

        item0.SplitView.preferredWidth = 25
        verify(isPolishScheduled(control))
        verify(waitForItemPolished(control))
        compare(item0.x, 0)
        compare(item0.y, 0)
        compare(item0.width, 25)
        compare(item0.height, testCase.height)
        compare(handle0.x, item0.width)
        compare(handle0.y, 0)
        compare(handle0.width, defaultHorizontalHandleWidth)
        compare(handle0.height, testCase.height)
        compare(item1.x, 25 + defaultHorizontalHandleWidth)
        compare(item1.y, 0)
        compare(item1.width, 0)
        compare(item1.height, testCase.height)
        compare(handle1.x, item1.x + item1.width)
        compare(handle1.y, 0)
        compare(handle1.width, defaultHorizontalHandleWidth)
        compare(handle1.height, testCase.height)
        compare(item2.x, item1.x + item1.width + defaultHorizontalHandleWidth)
        compare(item2.y, 0)
        compare(item2.width, testCase.width - item2.x)
        compare(item2.height, testCase.height)

        item0.SplitView.minimumWidth = 50
        verify(isPolishScheduled(control))
        verify(waitForItemPolished(control))
        compare(item0.x, 0)
        compare(item0.y, 0)
        compare(item0.width, 50)
        compare(item0.height, testCase.height)
        compare(handle0.x, item0.width)
        compare(handle0.y, 0)
        compare(handle0.width, defaultHorizontalHandleWidth)
        compare(handle0.height, testCase.height)
        compare(item1.x, 50 + defaultHorizontalHandleWidth)
        compare(item1.y, 0)
        compare(item1.width, 0)
        compare(item1.height, testCase.height)
        compare(handle1.x, item1.x + item1.width)
        compare(handle1.y, 0)
        compare(handle1.width, defaultHorizontalHandleWidth)
        compare(handle1.height, testCase.height)
        compare(item2.x, item1.x + item1.width + defaultHorizontalHandleWidth)
        compare(item2.y, 0)
        compare(item2.width, testCase.width - item2.x)
        compare(item2.height, testCase.height)

        item0.SplitView.preferredWidth = 100
        verify(isPolishScheduled(control))
        verify(waitForItemPolished(control))
        compare(item0.x, 0)
        compare(item0.y, 0)
        compare(item0.width, 100)
        compare(item0.height, testCase.height)
        compare(handle0.x, item0.width)
        compare(handle0.y, 0)
        compare(handle0.width, defaultHorizontalHandleWidth)
        compare(handle0.height, testCase.height)
        compare(item1.x, 100 + defaultHorizontalHandleWidth)
        compare(item1.y, 0)
        compare(item1.width, 0)
        compare(item1.height, testCase.height)
        compare(handle1.x, item1.x + item1.width)
        compare(handle1.y, 0)
        compare(handle1.width, defaultHorizontalHandleWidth)
        compare(handle1.height, testCase.height)
        compare(item2.x, item1.x + item1.width + defaultHorizontalHandleWidth)
        compare(item2.y, 0)
        compare(item2.width, testCase.width - item2.x)
        compare(item2.height, testCase.height)

        item0.SplitView.maximumWidth = 75
        verify(isPolishScheduled(control))
        verify(waitForItemPolished(control))
        compare(item0.x, 0)
        compare(item0.y, 0)
        compare(item0.width, 75)
        compare(item0.height, testCase.height)
        compare(handle0.x, item0.width)
        compare(handle0.y, 0)
        compare(handle0.width, defaultHorizontalHandleWidth)
        compare(handle0.height, testCase.height)
        compare(item1.x, 75 + defaultHorizontalHandleWidth)
        compare(item1.y, 0)
        compare(item1.width, 0)
        compare(item1.height, testCase.height)
        compare(handle1.x, item1.x + item1.width)
        compare(handle1.y, 0)
        compare(handle1.width, defaultHorizontalHandleWidth)
        compare(handle1.height, testCase.height)
        compare(item2.x, item1.x + item1.width + defaultHorizontalHandleWidth)
        compare(item2.y, 0)
        compare(item2.width, testCase.width - item2.x)
        compare(item2.height, testCase.height)

        item1.SplitView.fillWidth = true
        verify(isPolishScheduled(control))
        verify(waitForItemPolished(control))
        compare(item0.x, 0)
        compare(item0.y, 0)
        compare(item0.width, 75)
        compare(item0.height, testCase.height)
        compare(handle0.x, item0.width)
        compare(handle0.y, 0)
        compare(handle0.width, defaultHorizontalHandleWidth)
        compare(handle0.height, testCase.height)
        compare(item1.x, 75 + defaultHorizontalHandleWidth)
        compare(item1.y, 0)
        compare(item1.width, testCase.width - 75 - defaultHorizontalHandleWidth * 2)
        compare(item1.height, testCase.height)
        compare(handle1.x, item1.x + item1.width)
        compare(handle1.y, 0)
        compare(handle1.width, defaultHorizontalHandleWidth)
        compare(handle1.height, testCase.height)
        compare(item2.x, testCase.width)
        compare(item2.y, 0)
        compare(item2.width, 0)
        compare(item2.height, testCase.height)
    }

    Component {
        id: itemComponent
        Item {}
    }

    Component {
        id: objectComponent
        QtObject {}
    }

    function test_useAttachedPropertiesIncorrectly_data() {
        var properties = [ "fillWidth", "fillHeight", "minimumWidth", "minimumHeight",
            "preferredWidth", "preferredHeight",  "maximumWidth", "maximumHeight" ]

        var data = []

        for (var i = 0; i < properties.length; ++i) {
            var property = properties[i]
            data.push({ tag: "Item," + property, component: itemComponent, property: property,
                expectedWarning: /.*SplitView: attached properties must be accessed through a direct child of SplitView/ })
        }

        for (i = 0; i < properties.length; ++i) {
            property = properties[i]
            data.push({ tag: "QtObject," + property, component: objectComponent, property: property,
                expectedWarning: /.*SplitView: attached properties can only be used on Items/ })
        }

        return data
    }

    function test_useAttachedPropertiesIncorrectly(data) {
        // The object (whatever it may be) is not managed by a SplitView.
        var object = createTemporaryObject(data.component, testCase, { objectName: data.tag })
        verify(object)

        ignoreWarning(data.expectedWarning)
        // Should warn, but not crash.
        object.SplitView[data.property] = 1;
    }

    function test_sizes_data() {
        var splitViewWidth = testCase.width
        var splitViewHeight = testCase.height
        var data = [
            {
                // When the combined size of items is too large, the non-fill items should just exceed
                // the size of the SplitView, exactly as they would were they in a RowLayout, for example.
                tag: "fillItemOnLeft",
                expectedGeometries: [
                    // We're the fill item, but since the combined implicitWidths
                    // of the other two items take up all the space, we get none.
                    { x: 0, y: 0, width: 0, height: splitViewHeight },
                    // First handle.
                    { x: 0, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    // The second item does not fill, so its width should be unchanged.
                    { x: defaultHorizontalHandleWidth, y: 0, width: 200, height: splitViewHeight },
                    // Second handle.
                    { x: 200 + defaultHorizontalHandleWidth, y: 0, width: defaultHorizontalHandleWidth,
                        height: splitViewHeight },
                    // The third item also gets its implicitWidth.
                    { x: 200 + defaultHorizontalHandleWidth * 2, y: 0, width: 200, height: splitViewHeight }
                ]
            },
            {
                // Same as above except vertical.
                tag: "fillItemOnTop",
                expectedGeometries: [
                    // We're the fill item, but since the combined implicitHeights
                    // of the other two items take up all the space, we get none.
                    { x: 0, y: 0, width: splitViewWidth, height: 0 },
                    // First handle.
                    { x: 0, y: 0, width: splitViewWidth, height: defaultVerticalHandleHeight },
                    // The second item does not fill, so its height should be unchanged.
                    { x: 0, y: defaultVerticalHandleHeight, width: splitViewWidth, height: 200 },
                    // Second handle.
                    { x: 0, y: 200 + defaultVerticalHandleHeight, width: splitViewWidth,
                        height: defaultVerticalHandleHeight },
                    // The third item also gets its implicitHeight.
                    { x: 0, y: 200 + defaultVerticalHandleHeight * 2, width: splitViewWidth, height: 200 }
                ]
            },
            {
                tag: "fillItemInMiddle",
                expectedGeometries: [
                    // Our size is fixed.
                    { x: 0, y: 0, width: 25, height: splitViewHeight },
                    // First handle.
                    { x: 25, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    // The second item fills.
                    { x: 25 + defaultHorizontalHandleWidth, y: 0,
                        width: splitViewWidth - 25 - 200 - defaultHorizontalHandleWidth * 2, height: splitViewHeight },
                    // Second handle.
                    { x: splitViewWidth - 200 - defaultHorizontalHandleWidth, y: 0,
                        width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    // The third item's size is also fixed.
                    { x: splitViewWidth - 200, y: 0, width: 200, height: splitViewHeight }
                ]
            }
        ]
        return data
    }

    function test_sizes(data) {
        var component = Qt.createComponent("splitview/" + data.tag + ".qml")
        compare(component.status, Component.Ready, component.errorString());
        var control = createTemporaryObject(component, testCase, { "handle": handleComponent })
        verify(control)

        compareSizes(control, data.expectedGeometries)
    }

    Component {
        id: threeSizedItemsComponent

        SplitView {
            anchors.fill: parent
            handle: handleComponent

            Rectangle {
                objectName: "salmon"
                color: objectName
                implicitWidth: 25
                implicitHeight: 25
            }
            Rectangle {
                objectName: "navajowhite"
                color: objectName
                implicitWidth: 100
                implicitHeight: 100
            }
            Rectangle {
                objectName: "steelblue"
                color: objectName
                implicitWidth: 200
                implicitHeight: 200
            }
        }
    }

    function test_resetAttachedProperties_data() {
        var splitViewWidth = testCase.width
        var splitViewHeight = testCase.height
        var data = [
            {
                tag: "resetMinimumWidth",
                orientation: Qt.Horizontal,
                // Set the minimumWidth to 50. It should be used instead of implicitWidth since it's greater than 25.
                splitItemIndex: 0,
                propertyName: "minimumWidth",
                propertyValue: 50,
                expectedGeometriesBefore: [
                    { x: 0, y: 0, width: 50, height: splitViewHeight },
                    { x: 50, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: 50 + defaultHorizontalHandleWidth, y: 0, width: 100, height: splitViewHeight },
                    { x: 50 + 100 + defaultHorizontalHandleWidth, y: 0, width: defaultHorizontalHandleWidth,
                        height: splitViewHeight },
                    { x: 50 + 100 + defaultHorizontalHandleWidth * 2, y: 0,
                        width: splitViewWidth - 50 - 100 - defaultHorizontalHandleWidth * 2, height: splitViewHeight }
                ],
                // minimumWidth is now undefined, so implicitWidth should be used instead.
                expectedGeometriesAfter: [
                    { x: 0, y: 0, width: 25, height: splitViewHeight },
                    { x: 25, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: 25 + defaultHorizontalHandleWidth, y: 0, width: 100, height: splitViewHeight },
                    { x: 25 + 100 + defaultHorizontalHandleWidth, y: 0, width: defaultHorizontalHandleWidth,
                        height: splitViewHeight },
                    { x: 25 + 100 + defaultHorizontalHandleWidth * 2, y: 0,
                        width: splitViewWidth - 25 - 100 - defaultHorizontalHandleWidth * 2, height: splitViewHeight }
                ]
            },
            {
                tag: "resetMinimumHeight",
                orientation: Qt.Vertical,
                // Set the minimumHeight to 50. It should be used instead of implicitHeight since it's greater than 25.
                splitItemIndex: 0,
                propertyName: "minimumHeight",
                propertyValue: 50,
                expectedGeometriesBefore: [
                    { x: 0, y: 0, width: splitViewWidth, height: 50 },
                    { x: 0, y: 50, width: splitViewWidth, height: defaultVerticalHandleHeight },
                    { x: 0, y: 50 + defaultVerticalHandleHeight, width: splitViewWidth, height: 100 },
                    { x: 0, y: 50 + 100 + defaultVerticalHandleHeight, width: splitViewWidth,
                        height: defaultVerticalHandleHeight },
                    { x: 0, y: 50 + 100 + defaultVerticalHandleHeight * 2, width: splitViewWidth,
                        height: splitViewHeight - 50 - 100 - defaultVerticalHandleHeight * 2 }
                ],
                // preferredHeight is now undefined, so implicitHeight should be used instead.
                expectedGeometriesAfter: [
                    { x: 0, y: 0, width: splitViewWidth, height: 25 },
                    { x: 0, y: 25, width: splitViewWidth, height: defaultVerticalHandleHeight },
                    { x: 0, y: 25 + defaultVerticalHandleHeight, width: splitViewWidth, height: 100 },
                    { x: 0, y: 25 + 100 + defaultVerticalHandleHeight, width: splitViewWidth,
                        height: defaultVerticalHandleHeight },
                    { x: 0, y: 25 + 100 + defaultVerticalHandleHeight * 2, width: splitViewWidth,
                        height: splitViewHeight - 25 - 100 - defaultVerticalHandleHeight * 2 }
                ]
            },
            {
                tag: "resetPreferredWidth",
                orientation: Qt.Horizontal,
                // Set the preferredWidth to 50; it should be used instead of implicitWidth.
                splitItemIndex: 0,
                propertyName: "preferredWidth",
                propertyValue: 50,
                expectedGeometriesBefore: [
                    { x: 0, y: 0, width: 50, height: splitViewHeight },
                    { x: 50, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: 50 + defaultHorizontalHandleWidth, y: 0, width: 100, height: splitViewHeight },
                    { x: 50 + 100 + defaultHorizontalHandleWidth, y: 0, width: defaultHorizontalHandleWidth,
                        height: splitViewHeight },
                    { x: 50 + 100 + defaultHorizontalHandleWidth * 2, y: 0,
                        width: splitViewWidth - 50 - 100 - defaultHorizontalHandleWidth * 2, height: splitViewHeight }
                ],
                // preferredWidth is now undefined, so implicitWidth should be used instead.
                expectedGeometriesAfter: [
                    { x: 0, y: 0, width: 25, height: splitViewHeight },
                    { x: 25, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: 25 + defaultHorizontalHandleWidth, y: 0, width: 100, height: splitViewHeight },
                    { x: 25 + 100 + defaultHorizontalHandleWidth, y: 0, width: defaultHorizontalHandleWidth,
                        height: splitViewHeight },
                    { x: 25 + 100 + defaultHorizontalHandleWidth * 2, y: 0,
                        width: splitViewWidth - 25 - 100 - defaultHorizontalHandleWidth * 2, height: splitViewHeight }
                ]
            },
            {
                tag: "resetPreferredHeight",
                orientation: Qt.Vertical,
                // Set the preferredHeight to 50; it should be used instead of implicitHeight.
                splitItemIndex: 0,
                propertyName: "preferredHeight",
                propertyValue: 50,
                expectedGeometriesBefore: [
                    { x: 0, y: 0, width: splitViewWidth, height: 50 },
                    { x: 0, y: 50, width: splitViewWidth, height: defaultVerticalHandleHeight },
                    { x: 0, y: 50 + defaultVerticalHandleHeight, width: splitViewWidth, height: 100 },
                    { x: 0, y: 50 + 100 + defaultVerticalHandleHeight, width: splitViewWidth,
                        height: defaultVerticalHandleHeight },
                    { x: 0, y: 50 + 100 + defaultVerticalHandleHeight * 2, width: splitViewWidth,
                        height: splitViewHeight - 50 - 100 - defaultVerticalHandleHeight * 2 }
                ],
                // preferredHeight is now undefined, so implicitHeight should be used instead.
                expectedGeometriesAfter: [
                    { x: 0, y: 0, width: splitViewWidth, height: 25 },
                    { x: 0, y: 25, width: splitViewWidth, height: defaultVerticalHandleHeight },
                    { x: 0, y: 25 + defaultVerticalHandleHeight, width: splitViewWidth, height: 100 },
                    { x: 0, y: 25 + 100 + defaultVerticalHandleHeight, width: splitViewWidth,
                        height: defaultVerticalHandleHeight },
                    { x: 0, y: 25 + 100 + defaultVerticalHandleHeight * 2, width: splitViewWidth,
                        height: splitViewHeight - 25 - 100 - defaultVerticalHandleHeight * 2 }
                ]
            },
            {
                tag: "resetMaximumWidth",
                orientation: Qt.Horizontal,
                // Set the maximumWidth to 15. It should be used instead of implicitWidth since it's less than 25.
                splitItemIndex: 0,
                propertyName: "maximumWidth",
                propertyValue: 15,
                expectedGeometriesBefore: [
                    { x: 0, y: 0, width: 15, height: splitViewHeight },
                    { x: 15, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: 15 + defaultHorizontalHandleWidth, y: 0, width: 100, height: splitViewHeight },
                    { x: 15 + 100 + defaultHorizontalHandleWidth, y: 0, width: defaultHorizontalHandleWidth,
                        height: splitViewHeight },
                    { x: 15 + 100 + defaultHorizontalHandleWidth * 2, y: 0,
                        width: splitViewWidth - 15 - 100 - defaultHorizontalHandleWidth * 2, height: splitViewHeight }
                ],
                // maximumWidth is now undefined, so implicitWidth should be used instead.
                expectedGeometriesAfter: [
                    { x: 0, y: 0, width: 25, height: splitViewHeight },
                    { x: 25, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: 25 + defaultHorizontalHandleWidth, y: 0, width: 100, height: splitViewHeight },
                    { x: 25 + 100 + defaultHorizontalHandleWidth, y: 0, width: defaultHorizontalHandleWidth,
                        height: splitViewHeight },
                    { x: 25 + 100 + defaultHorizontalHandleWidth * 2, y: 0,
                        width: splitViewWidth - 25 - 100 - defaultHorizontalHandleWidth * 2, height: splitViewHeight }
                ]
            },
            {
                tag: "resetMaximumHeight",
                orientation: Qt.Vertical,
                // Set the preferredHeight to 15. It should be used instead of implicitHeight if it's not undefined.
                splitItemIndex: 0,
                propertyName: "maximumHeight",
                propertyValue: 15,
                expectedGeometriesBefore: [
                    { x: 0, y: 0, width: splitViewWidth, height: 15 },
                    { x: 0, y: 15, width: splitViewWidth, height: defaultVerticalHandleHeight },
                    { x: 0, y: 15 + defaultVerticalHandleHeight, width: splitViewWidth, height: 100 },
                    { x: 0, y: 15 + 100 + defaultVerticalHandleHeight, width: splitViewWidth,
                        height: defaultVerticalHandleHeight },
                    { x: 0, y: 15 + 100 + defaultVerticalHandleHeight * 2, width: splitViewWidth,
                        height: splitViewHeight - 15 - 100 - defaultVerticalHandleHeight * 2 }
                ],
                // preferredHeight is now undefined, so implicitHeight should be used instead.
                expectedGeometriesAfter: [
                    { x: 0, y: 0, width: splitViewWidth, height: 25 },
                    { x: 0, y: 25, width: splitViewWidth, height: defaultVerticalHandleHeight },
                    { x: 0, y: 25 + defaultVerticalHandleHeight, width: splitViewWidth, height: 100 },
                    { x: 0, y: 25 + 100 + defaultVerticalHandleHeight, width: splitViewWidth,
                        height: defaultVerticalHandleHeight },
                    { x: 0, y: 25 + 100 + defaultVerticalHandleHeight * 2, width: splitViewWidth,
                        height: splitViewHeight - 25 - 100 - defaultVerticalHandleHeight * 2 }
                ]
            },
        ]
        return data;
    }

    function test_resetAttachedProperties(data) {
        var control = createTemporaryObject(threeSizedItemsComponent, testCase,
            { "orientation": data.orientation })
        verify(control)

        var splitItem = control.itemAt(data.splitItemIndex)
        splitItem.SplitView[data.propertyName] = data.propertyValue
        verify(isPolishScheduled(control))
        verify(waitForItemPolished(control))
        compareSizes(control, data.expectedGeometriesBefore, "after setting attached property")

        splitItem.SplitView[data.propertyName] = undefined
        verify(isPolishScheduled(control))
        verify(waitForItemPolished(control))
        compareSizes(control, data.expectedGeometriesAfter, "after resetting attached property")
    }

    function test_orientation() {
        var control = createTemporaryObject(threeSizedItemsComponent, testCase)
        verify(control)

        var item0 = control.itemAt(0)
        compare(item0.x, 0)
        compare(item0.y, 0)
        compare(item0.width, item0.implicitWidth)
        compare(item0.height, testCase.height)

        var handles = findHandles(control)
        var handle0 = handles[0]
        compare(handle0.x, item0.implicitWidth)
        compare(handle0.y, 0)
        compare(handle0.width, defaultHorizontalHandleWidth)
        compare(handle0.height, testCase.height)

        var item1 = control.itemAt(1)
        compare(item1.x, item0.width + defaultHorizontalHandleWidth)
        compare(item1.y, 0)
        compare(item1.width, item1.implicitWidth)
        compare(item1.height, testCase.height)

        var handle1 = handles[1]
        compare(handle1.x, item1.x + item1.width)
        compare(handle1.y, 0)
        compare(handle1.width, defaultHorizontalHandleWidth)
        compare(handle1.height, testCase.height)

        var item2 = control.itemAt(2)
        compare(item2.x, item0.width + item1.width + defaultHorizontalHandleWidth * 2)
        compare(item2.y, 0)
        compare(item2.width, testCase.width - item2.x)
        compare(item2.height, testCase.height)

        control.orientation = Qt.Vertical
        verify(isPolishScheduled(control))
        verify(waitForItemPolished(control))
        compare(item0.x, 0)
        compare(item0.y, 0)
        compare(item0.width, testCase.width)
        compare(item0.height, item0.implicitHeight)
        handles = findHandles(control)
        handle0 = handles[0]
        compare(handle0.x, 0)
        compare(handle0.y, item0.implicitHeight)
        compare(handle0.width, testCase.width)
        compare(handle0.height, defaultVerticalHandleHeight)
        compare(item1.x, 0)
        compare(item1.y, item0.height + defaultVerticalHandleHeight)
        compare(item1.width, testCase.width)
        compare(item1.height, item1.implicitHeight)
        handle1 = handles[1]
        compare(handle1.x, 0)
        compare(handle1.y, item1.y + item1.height)
        compare(handle1.width, testCase.width)
        compare(handle1.height, defaultVerticalHandleHeight)
        compare(item2.x, 0)
        compare(item2.y, item0.height + item1.height + defaultVerticalHandleHeight * 2)
        compare(item2.width, testCase.width)
        compare(item2.height, testCase.height - item2.y)
    }

    readonly property int splitViewMargins: 50

    Component {
        id: threeItemsMinSizeAndFillComponent

        SplitView {
            anchors.fill: parent
            handle: handleComponent

            Rectangle {
                objectName: "salmon"
                color: objectName
                implicitWidth: 25
                implicitHeight: 25
                SplitView.minimumWidth: 25
                SplitView.minimumHeight: 25
                SplitView.fillWidth: true
                SplitView.fillHeight: true
            }
            Rectangle {
                objectName: "navajowhite"
                color: objectName
                implicitWidth: 100
                implicitHeight: 100
            }
            Rectangle {
                objectName: "steelblue"
                color: objectName
                implicitWidth: 200
                implicitHeight: 200
            }
        }
    }

    Component {
        id: repeaterSplitViewComponent

        SplitView {
            anchors.fill: parent
            handle: handleComponent

            property alias repeater: repeater

            Repeater {
                id: repeater
                model: 3
                delegate: Rectangle {
                    objectName: "rectDelegate" + index

                    SplitView.preferredWidth: 25

                    color: "#aaff0000"

                    Text {
                        text: parent.x + "," + parent.y + " " + parent.width + "x" + parent.height
                        color: "white"
                        rotation: 90
                        anchors.centerIn: parent
                    }
                }
            }
        }
    }

    Component {
        id: hiddenItemSplitViewComponent

        SplitView {
            anchors.fill: parent
            handle: handleComponent

            Rectangle {
                objectName: "steelblue"
                color: objectName

                SplitView.minimumWidth: 50
            }
            Rectangle {
                objectName: "tomato"
                color: objectName

                SplitView.fillWidth: true
                SplitView.preferredWidth: 200
            }
            Rectangle {
                objectName: "navajowhite"
                color: objectName
                visible: false

                SplitView.minimumWidth: visible ? 100 : 0
            }
            Rectangle {
                objectName: "mediumseagreen"
                color: objectName

                SplitView.minimumWidth: 50
            }
        }
    }

    function test_dragHandle_data() {
        var splitViewWidth = testCase.width - splitViewMargins * 2
        var splitViewHeight = testCase.height - splitViewMargins * 2
        var data = [
            {
                tag: "fillThirdItemAndDragFirstHandlePastRightSide",
                component: threeSizedItemsComponent,
                orientation: Qt.Horizontal,
                // The index of the item that will fill.
                fillIndex: 2,
                // The index of the handle to be dragged.
                handleIndex: 0,
                // The position where the center of the handle will be.
                newHandlePos: Qt.point(testCase.width + 20, testCase.height / 2),
                // The expected geometry of each item managed by the SplitView before dragging the handle.
                expectedGeometriesBeforeDrag: [
                    // First item.
                    { x: 0, y: 0, width: 25, height: splitViewHeight },
                    // First handle.
                    { x: 25, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    // Second item.
                    { x: 25 + defaultHorizontalHandleWidth, y: 0, width: 100, height: splitViewHeight },
                    // Second handle.
                    { x: 25 + 100 + defaultHorizontalHandleWidth, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    // Third item (fills).
                    { x: 25 + 100 + defaultHorizontalHandleWidth * 2, y: 0,
                        width: splitViewWidth - 25 - 100 - defaultHorizontalHandleWidth * 2, height: splitViewHeight }
                ],
                // The expected geometry of each item managed by the SplitView after dragging the handle.
                expectedGeometriesAfterDrag: [
                    // The fill item is to the right of the handle at index 0, so the handle belongs
                    // to the left item: us. We should consume all of the fill item's width.
                    { x: 0, y: 0, width: splitViewWidth - 100 - defaultHorizontalHandleWidth * 2,
                        height: splitViewHeight },
                    // First handle.
                    { x: splitViewWidth - defaultHorizontalHandleWidth * 2 - 100,
                        y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    // The second item does not fill, so its width should be unchanged.
                    { x: splitViewWidth - 100 - defaultHorizontalHandleWidth,
                        y: 0, width: 100, height: splitViewHeight },
                    // Second handle.
                    { x: splitViewWidth - defaultHorizontalHandleWidth,
                        y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    // The last item does fill, so it should lose all of its width.
                    { x: splitViewWidth, y: 0, width: 0, height: splitViewHeight }
                ]
            },
            {
                tag: "fillThirdItemAndDragFirstHandlePastBottomSide",
                component: threeSizedItemsComponent,
                orientation: Qt.Vertical,
                fillIndex: 2,
                handleIndex: 0,
                newHandlePos: Qt.point(testCase.width / 2, testCase.height + 20),
                expectedGeometriesBeforeDrag: [
                    { x: 0, y: 0, width: splitViewWidth, height: 25 },
                    { x: 0, y: 25, width: splitViewWidth, height: defaultVerticalHandleHeight },
                    { x: 0, y: 25 + defaultVerticalHandleHeight, width: splitViewWidth, height: 100 },
                    { x: 0, y: 25 + 100 + defaultVerticalHandleHeight, width: splitViewWidth, height: defaultVerticalHandleHeight },
                    { x: 0, y: 25 + 100 + defaultVerticalHandleHeight * 2,
                        width: splitViewWidth, height: splitViewHeight - 25 - 100 - defaultVerticalHandleHeight * 2 }
                ],
                // The expected geometry of each item managed by the SplitView after dragging the handle.
                expectedGeometriesAfterDrag: [
                    // The fill item is to the bottom of the handle at index 0, so the handle belongs
                    // to the top item: us. We should consume all of the fill item's width.
                    { x: 0, y: 0, width: splitViewWidth,
                        height: splitViewHeight - 100 - defaultVerticalHandleHeight * 2 },
                    // First handle.
                    { x: 0, y: splitViewHeight - defaultVerticalHandleHeight * 2 - 100,
                        width: splitViewWidth, height: defaultVerticalHandleHeight },
                    // The second item does not fill, so its height should be unchanged.
                    { x: 0, y: splitViewWidth - 100 - defaultVerticalHandleHeight,
                        width: splitViewWidth, height: 100 },
                    // Second handle.
                    { x: 0, y: splitViewHeight - defaultVerticalHandleHeight,
                        width: splitViewWidth, height: defaultVerticalHandleHeight },
                    // The last item does fill, so it should lose all of its width.
                    { x: 0, y: splitViewHeight, width: splitViewWidth, height: 0 }
                ]
            },
            {
                tag: "fillThirdItemAndDragSecondHandlePastLeftSide",
                component: threeSizedItemsComponent,
                orientation: Qt.Horizontal,
                fillIndex: 2,
                handleIndex: 1,
                newHandlePos: Qt.point(-20, testCase.height / 2),
                expectedGeometriesBeforeDrag: [
                    { x: 0, y: 0, width: 25, height: splitViewHeight },
                    { x: 25, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: 25 + defaultHorizontalHandleWidth, y: 0, width: 100, height: splitViewHeight },
                    { x: 25 + 100 + defaultHorizontalHandleWidth, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: 25 + 100 + defaultHorizontalHandleWidth * 2, y: 0,
                        width: splitViewWidth - 25 - 100 - defaultHorizontalHandleWidth * 2, height: splitViewHeight }
                ],
                expectedGeometriesAfterDrag: [
                    // The fill item is to the right of the handle at index 1, so the handle belongs
                    // to the second item; our width should be unchanged.
                    { x: 0, y: 0, width: 25, height: splitViewHeight },
                    // First handle.
                    { x: 25, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    // The second item is the one being resized, and since we're dragging its handle
                    // to the left, its width should decrease.
                    { x: 25 + defaultHorizontalHandleWidth, y: 0, width: 0, height: splitViewHeight },
                    // Second handle.
                    { x: 25 + defaultHorizontalHandleWidth, y: 0, width: defaultHorizontalHandleWidth,
                        height: splitViewHeight },
                    // The last item fills, so it should get the second item's lost width.
                    { x: 25 + defaultHorizontalHandleWidth * 2, y: 0,
                        width: splitViewWidth - 25 - defaultHorizontalHandleWidth * 2, height: splitViewHeight }
                ]
            },
            {
                tag: "fillThirdItemAndDragSecondHandlePastTopSide",
                component: threeSizedItemsComponent,
                orientation: Qt.Vertical,
                fillIndex: 2,
                handleIndex: 1,
                newHandlePos: Qt.point(testCase.width / 2, -20),
                expectedGeometriesBeforeDrag: [
                    { x: 0, y: 0, width: splitViewWidth, height: 25 },
                    { x: 0, y: 25, width: splitViewWidth, height: defaultVerticalHandleHeight },
                    { x: 0, y: 25 + defaultVerticalHandleHeight, width: splitViewWidth, height: 100 },
                    { x: 0, y: 25 + 100 + defaultVerticalHandleHeight, width: splitViewWidth, height: defaultVerticalHandleHeight },
                    { x: 0, y: 25 + 100 + defaultVerticalHandleHeight * 2,
                        width: splitViewWidth, height: splitViewHeight - 25 - 100 - defaultVerticalHandleHeight * 2 }
                ],
                expectedGeometriesAfterDrag: [
                    // The fill item is to the bottom of the handle at index 1, so the handle belongs
                    // to the second item; our height should be unchanged.
                    { x: 0, y: 0, width: splitViewWidth, height: 25 },
                    // First handle.
                    { x: 0, y: 25, width: splitViewWidth, height: defaultVerticalHandleHeight },
                    // The second item is the one being resized, and since we're dragging its handle
                    // to the top, its height should decrease.
                    { x: 0, y: 25 + defaultVerticalHandleHeight, width: splitViewWidth, height: 0 },
                    // Second handle.
                    { x: 0, y: 25 + defaultVerticalHandleHeight, width: splitViewWidth,
                        height: defaultVerticalHandleHeight },
                    // The last item fills, so it should get the second item's lost height.
                    { x: 0, y: 25 + defaultVerticalHandleHeight * 2,
                        width: splitViewWidth, height: splitViewHeight - 25 - defaultVerticalHandleHeight * 2 }
                ]
            },
            {
                // First item should start off empty and then eventually take up all of 3rd item's space
                // as the handle is dragged past the right side.
                tag: "fillFirstItemAndDragSecondHandlePastRightSide",
                component: threeSizedItemsComponent,
                orientation: Qt.Horizontal,
                fillIndex: 0,
                handleIndex: 1,
                newHandlePos: Qt.point(testCase.width + 20, testCase.height / 2),
                expectedGeometriesBeforeDrag: [
                    { x: 0, y: 0, width: 0, height: splitViewHeight },
                    { x: 0, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: defaultHorizontalHandleWidth, y: 0, width: 100, height: splitViewHeight },
                    { x: 100 + defaultHorizontalHandleWidth, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: 100 + defaultHorizontalHandleWidth * 2, y: 0, width: 200, height: splitViewHeight }
                ],
                expectedGeometriesAfterDrag: [
                    // The fill item is to the left of the handle at index 1, so the handle belongs
                    // to the third item. Since we're moving the handle to the right side of the
                    // SplitView, our width should grow as we consume the width of the third item.
                    { x: 0, y: 0, width: splitViewWidth - 100 - defaultHorizontalHandleWidth * 2, height: splitViewHeight },
                    // First handle.
                    { x: splitViewWidth - 100 - defaultHorizontalHandleWidth * 2, y: 0,
                        width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    // The second item's width remains unchanged.
                    { x: splitViewWidth - 100 - defaultHorizontalHandleWidth, y: 0, width: 100, height: splitViewHeight },
                    // Second handle.
                    { x: splitViewWidth - defaultHorizontalHandleWidth, y: 0,
                        width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    // The last item loses its width.
                    { x: splitViewWidth, y: 0, width: 0, height: splitViewHeight }
                ]
            },
            {
                // First item should start off empty and then eventually take up all of 3rd item's space
                // as the handle is dragged past the bottom side.
                tag: "fillFirstItemAndDragSecondHandlePastBottomSide",
                component: threeSizedItemsComponent,
                orientation: Qt.Vertical,
                fillIndex: 0,
                handleIndex: 1,
                newHandlePos: Qt.point(testCase.width / 2, testCase.height + 20),
                expectedGeometriesBeforeDrag: [
                    { x: 0, y: 0, width: splitViewWidth, height: 0 },
                    { x: 0, y: 0, width: splitViewWidth, height: defaultVerticalHandleHeight },
                    { x: 0, y: defaultVerticalHandleHeight, width: splitViewWidth, height: 100 },
                    { x: 0, y: 100 + defaultVerticalHandleHeight, width: splitViewWidth, height: defaultVerticalHandleHeight },
                    { x: 0, y: 100 + defaultVerticalHandleHeight * 2, width: splitViewWidth, height: 200 }
                ],
                expectedGeometriesAfterDrag: [
                    // The fill item is to the top of the handle at index 1, so the handle belongs
                    // to the third item. Since we're moving the handle to the bottom side of the
                    // SplitView, our height should grow as we consume the height of the third item.
                    { x: 0, y: 0, width: splitViewWidth, height: splitViewHeight - 100 - defaultVerticalHandleHeight * 2 },
                    // First handle.
                    { x: 0, y: splitViewHeight - 100 - defaultVerticalHandleHeight * 2,
                        width: splitViewWidth, height: defaultVerticalHandleHeight },
                    // The second item's width remains unchanged.
                    { x: 0, y: splitViewHeight - 100 - defaultVerticalHandleHeight, width: splitViewWidth, height: 100 },
                    // Second handle.
                    { x: 0, y: splitViewHeight - defaultVerticalHandleHeight,
                        width: splitViewWidth, height: defaultVerticalHandleHeight },
                    // The last item loses its width.
                    { x: 0, y: splitViewHeight, width: splitViewHeight, height: 0 }
                ]
            },
            {
                tag: "fillFirstItemAndDragFirstHandlePastLeftSide",
                component: threeSizedItemsComponent,
                orientation: Qt.Horizontal,
                fillIndex: 0,
                handleIndex: 0,
                newHandlePos: Qt.point(-20, testCase.height / 2),
                expectedGeometriesBeforeDrag: [
                    { x: 0, y: 0, width: 0, height: splitViewHeight },
                    { x: 0, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: defaultHorizontalHandleWidth, y: 0, width: 100, height: splitViewHeight },
                    { x: 100 + defaultHorizontalHandleWidth, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    // The second item's implicitWidth is 100, and ours is 200. The available width is 300,
                    // so both items get their implicit widths.
                    { x: 100 + defaultHorizontalHandleWidth * 2, y: 0, width: splitViewWidth - 100, height: splitViewHeight }
                ],
                // Should be unchanged.
                expectedGeometriesAfterDrag: [
                    { x: 0, y: 0, width: 0, height: splitViewHeight },
                    { x: 0, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: defaultHorizontalHandleWidth, y: 0, width: 100, height: splitViewHeight },
                    { x: 100 + defaultHorizontalHandleWidth, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: 100 + defaultHorizontalHandleWidth * 2, y: 0, width: splitViewWidth - 100, height: splitViewHeight }
                ]
            },
            {
                tag: "fillFirstItemWithMinWidthAndDragFirstHandlePastLeftSide",
                component: threeItemsMinSizeAndFillComponent,
                orientation: Qt.Horizontal,
                fillIndex: 0,
                handleIndex: 0,
                newHandlePos: Qt.point(-20, testCase.height / 2),
                expectedGeometriesBeforeDrag: [
                    { x: 0, y: 0, width: 25, height: splitViewHeight },
                    { x: 25, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: 25 + defaultHorizontalHandleWidth, y: 0, width: 100, height: splitViewHeight },
                    { x: 25 + 100 + defaultHorizontalHandleWidth, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: 25 + 100 + defaultHorizontalHandleWidth * 2, y: 0, width: splitViewWidth - 100, height: splitViewHeight }
                ],
                // Should be unchanged.
                expectedGeometriesAfterDrag: [
                    { x: 0, y: 0, width: 25, height: splitViewHeight },
                    { x: 25, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: 25 + defaultHorizontalHandleWidth, y: 0, width: 100, height: splitViewHeight },
                    { x: 25 + 100 + defaultHorizontalHandleWidth, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: 25 + 100 + defaultHorizontalHandleWidth * 2, y: 0, width: splitViewWidth - 100, height: splitViewHeight }
                ]
            },
            {
                tag: "repeater",
                component: repeaterSplitViewComponent,
                orientation: Qt.Horizontal,
                fillIndex: 2,
                handleIndex: 1,
                newHandlePos: Qt.point(200, testCase.height / 2),
                expectedGeometriesBeforeDrag: [
                    { x: 0, y: 0, width: 25, height: splitViewHeight },
                    { x: 25, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: 25 + defaultHorizontalHandleWidth, y: 0, width: 25, height: splitViewHeight },
                    { x: 25 * 2 + defaultHorizontalHandleWidth, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: 25 * 2 + defaultHorizontalHandleWidth * 2, y: 0, width: splitViewWidth - 70 , height: splitViewHeight }
                ],
                expectedGeometriesAfterDrag: [
                    { x: 0, y: 0, width: 25, height: splitViewHeight },
                    { x: 25, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: 25 + defaultHorizontalHandleWidth, y: 0, width: 105, height: splitViewHeight },
                    { x: 140, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: 150, y: 0, width: 150, height: splitViewHeight }
                ]
            },
            {
                tag: "hiddenItemSplitViewComponent",
                // [50] | [200 (fill)] | [hidden] | [50]
                component: hiddenItemSplitViewComponent,
                orientation: Qt.Horizontal,
                fillIndex: 1,
                handleIndex: 1,
                // Drag to the horizontal centre of the SplitView.
                newHandlePos: Qt.point(splitViewMargins + 150, testCase.height / 2),
                expectedGeometriesBeforeDrag: [
                    { x: 0, y: 0, width: 50, height: splitViewHeight },
                    { x: 50, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: 50 + defaultHorizontalHandleWidth, y: 0, width: 200 - defaultHorizontalHandleWidth * 2, height: splitViewHeight },
                    { x: 250 - (defaultHorizontalHandleWidth * 2) + defaultHorizontalHandleWidth, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { hidden: true }, // Third item should be hidden.
                    { hidden: true }, // Handle for third item should be hidden.
                    { x: 250 - (defaultHorizontalHandleWidth * 2) + defaultHorizontalHandleWidth * 2, y: 0, width: 50, height: splitViewHeight }
                ],
                expectedGeometriesAfterDrag: [
                    { x: 0, y: 0, width: 50, height: splitViewHeight },
                    { x: 50, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    // Width of the fill item should end up smaller.
                    { x: 50 + defaultHorizontalHandleWidth, y: 0, width: 100 - defaultHorizontalHandleWidth * 2, height: splitViewHeight },
                    { x: 150 - (defaultHorizontalHandleWidth * 2) + defaultHorizontalHandleWidth, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { hidden: true }, // Third item should be hidden.
                    { hidden: true }, // Handle for third item should be hidden.
                    // Width of the last item should grow.
                    { x: 150 - (defaultHorizontalHandleWidth * 2) + defaultHorizontalHandleWidth * 2, y: 0, width: 150, height: splitViewHeight }
                ]
            }
        ]
        return data
    }

    function test_dragHandle(data) {
        var control = createTemporaryObject(data.component, testCase)
        verify(control)

        control.orientation = data.orientation

        // Ensure that there is space to drag outside of the SplitView.
        control.anchors.margins = splitViewMargins

        var fillItem = control.itemAt(data.fillIndex)
        if (control.orientation === Qt.Horizontal)
            fillItem.SplitView.fillWidth = true
        else
            fillItem.SplitView.fillHeight = true

        // Check the sizes (and visibility) of the items before the drag.
        verify(isPolishScheduled(control))
        verify(waitForItemPolished(control))
        compareSizes(control, data.expectedGeometriesBeforeDrag, "before drag")

        // Drag the handle.
        var handles = findHandles(control)
        var targetHandle = handles[data.handleIndex]
        verify(targetHandle.visible)
        mousePress(targetHandle)
        verify(control.resizing)
        // newHandlePos is in scene coordinates, so map it to coordinates local to the handle.
        var localPos = testCase.mapToItem(targetHandle, data.newHandlePos.x, data.newHandlePos.y)
        mouseMove(targetHandle, localPos.x - targetHandle.width / 2, localPos.y - targetHandle.height / 2)
        verify(control.resizing)
        compareSizes(control, data.expectedGeometriesAfterDrag, "after drag move")

        // The geometries should remain unchanged after releasing.
        mouseRelease(targetHandle, localPos.x - targetHandle.width / 2, localPos.y - targetHandle.height / 2, Qt.LeftButton)
        verify(!control.resizing)
        compareSizes(control, data.expectedGeometriesAfterDrag, "after drag release")
    }

    function test_splitViewGeometryChanges_data() {
        var defaultSplitViewWidth = testCase.width
        var defaultSplitViewHeight = testCase.height

        var data = [
            {
                tag: "growWidth",
                orientation: Qt.Horizontal,
                splitViewWidth: 800,
                expectedGeometries: [
                    { x: 0, y: 0, width: 25, height: defaultSplitViewHeight },
                    { x: 25, y: 0, width: defaultHorizontalHandleWidth, height: defaultSplitViewHeight },
                    { x: 25 + defaultHorizontalHandleWidth, y: 0, width: 100, height: defaultSplitViewHeight },
                    { x: 25 + 100 + defaultHorizontalHandleWidth, y: 0, width: defaultHorizontalHandleWidth,
                        height: defaultSplitViewHeight },
                    { x: 25 + 100 + defaultHorizontalHandleWidth * 2, y: 0,
                        width: 800 - 25 - 100 - defaultHorizontalHandleWidth * 2, height: defaultSplitViewHeight }
                ]
            },
            {
                // Same as above except vertical.
                tag: "growHeight",
                orientation: Qt.Vertical,
                splitViewHeight: 800,
                expectedGeometries: [
                    { x: 0, y: 0, width: defaultSplitViewWidth, height: 25 },
                    { x: 0, y: 25, width: defaultSplitViewWidth, height: defaultVerticalHandleHeight },
                    { x: 0, y: 25 + defaultVerticalHandleHeight, width: defaultSplitViewWidth, height: 100 },
                    { x: 0, y: 25 + 100 + defaultVerticalHandleHeight, width: defaultSplitViewWidth,
                        height: defaultVerticalHandleHeight },
                    { x: 0, y: 25 + 100 + defaultVerticalHandleHeight * 2, width: defaultSplitViewWidth,
                        height: 800 - 25 - 100 - defaultVerticalHandleHeight * 2 }
                ]
            },
            {
                tag: "shrinkWidth",
                orientation: Qt.Horizontal,
                splitViewWidth: 200,
                expectedGeometries: [
                    { x: 0, y: 0, width: 25, height: defaultSplitViewHeight },
                    { x: 25, y: 0, width: defaultHorizontalHandleWidth, height: defaultSplitViewHeight },
                    { x: 25 + defaultHorizontalHandleWidth, y: 0, width: 100, height: defaultSplitViewHeight },
                    { x: 25 + 100 + defaultHorizontalHandleWidth, y: 0, width: defaultHorizontalHandleWidth,
                        height: defaultSplitViewHeight },
                    { x: 25 + 100 + defaultHorizontalHandleWidth * 2, y: 0,
                        width: 200 - 25 - 100 - defaultHorizontalHandleWidth * 2, height: defaultSplitViewHeight }
                ]
            },
            {
                // Same as above except vertical.
                tag: "shrinkHeight",
                orientation: Qt.Vertical,
                splitViewHeight: 200,
                expectedGeometries: [
                    { x: 0, y: 0, width: defaultSplitViewWidth, height: 25 },
                    { x: 0, y: 25, width: defaultSplitViewWidth, height: defaultVerticalHandleHeight },
                    { x: 0, y: 25 + defaultVerticalHandleHeight, width: defaultSplitViewWidth, height: 100 },
                    { x: 0, y: 25 + 100 + defaultVerticalHandleHeight, width: defaultSplitViewWidth,
                        height: defaultVerticalHandleHeight },
                    { x: 0, y: 25 + 100 + defaultVerticalHandleHeight * 2, width: defaultSplitViewWidth,
                        height: 200 - 25 - 100 - defaultVerticalHandleHeight * 2 }
                ]
            },
        ]
        return data
    }

    function test_splitViewGeometryChanges(data) {
        var control = createTemporaryObject(threeSizedItemsComponent, testCase,
            { "handle": handleComponent, "anchors.fill": undefined, "orientation": data.orientation })
        verify(control)

        if (data.hasOwnProperty("splitViewWidth"))
            control.width = data.splitViewWidth
        else
            control.width = testCase.width

        if (data.hasOwnProperty("splitViewHeight"))
            control.height = data.splitViewHeight
        else
            control.height = testCase.height

        verify(isPolishScheduled(control))
        verify(waitForItemPolished(control))
        compareSizes(control, data.expectedGeometries)
    }

    function test_splitItemImplicitSizeChanges_data() {
        var defaultSplitViewWidth = testCase.width
        var defaultSplitViewHeight = testCase.height

        var data = [
            {
                tag: "growImplicitWidth",
                orientation: Qt.Horizontal,
                splitItemImplicitWidth: 50,
                expectedGeometries: [
                    { x: 0, y: 0, width: 50, height: defaultSplitViewHeight },
                    { x: 50, y: 0, width: defaultHorizontalHandleWidth, height: defaultSplitViewHeight },
                    { x: 50 + defaultHorizontalHandleWidth, y: 0, width: 100, height: defaultSplitViewHeight },
                    { x: 50 + 100 + defaultHorizontalHandleWidth, y: 0, width: defaultHorizontalHandleWidth,
                        height: defaultSplitViewHeight },
                    { x: 50 + 100 + defaultHorizontalHandleWidth * 2, y: 0,
                        width: defaultSplitViewWidth - 50 - 100 - defaultHorizontalHandleWidth * 2, height: defaultSplitViewHeight }
                ]
            },
            {
                tag: "growImplicitHeight",
                orientation: Qt.Vertical,
                splitItemImplicitHeight: 50,
                expectedGeometries: [
                    { x: 0, y: 0, width: defaultSplitViewWidth, height: 50 },
                    { x: 0, y: 50, width: defaultSplitViewWidth, height: defaultVerticalHandleHeight },
                    { x: 0, y: 50 + defaultVerticalHandleHeight, width: defaultSplitViewWidth, height: 100 },
                    { x: 0, y: 50 + 100 + defaultVerticalHandleHeight, width: defaultSplitViewWidth,
                        height: defaultVerticalHandleHeight },
                    { x: 0, y: 50 + 100 + defaultVerticalHandleHeight * 2, width: defaultSplitViewWidth,
                        height: defaultSplitViewHeight - 50 - 100 - defaultVerticalHandleHeight * 2 }
                ]
            }
        ]
        return data
    }

    // Tests that implicitWidth/Height changes in items are noticed by SplitView.
    function test_splitItemImplicitSizeChanges(data) {
        var control = createTemporaryObject(threeSizedItemsComponent, testCase,
            { "handle": handleComponent, "orientation": data.orientation })
        verify(control)

        var firstItem = control.itemAt(0)

        if (data.hasOwnProperty("splitItemImplicitWidth"))
            firstItem.implicitWidth = data.splitItemImplicitWidth

        if (data.hasOwnProperty("splitItemImplicitHeight"))
            firstItem.implicitHeight = data.splitItemImplicitHeight

        verify(isPolishScheduled(control))
        verify(waitForItemPolished(control))
        compareSizes(control, data.expectedGeometries)
    }

    Component {
        id: largerHandle
        Rectangle {
            objectName: "largerHandle"
            implicitWidth: 20
            implicitHeight: 20
            color: "#444"
        }
    }

    Component {
        id: smallerHandle
        Rectangle {
            objectName: "smallerHandle"
            implicitWidth: 5
            implicitHeight: 5
            color: "#444"
        }
    }

    function test_handleChanges_data() {
        var splitViewWidth = testCase.width
        var splitViewHeight = testCase.height

        var data = [
            {
                tag: "growHandleWidth",
                orientation: Qt.Horizontal,
                handleComponent: largerHandle,
                expectedGeometries: [
                    { x: 0, y: 0, width: 25, height: splitViewHeight },
                    { x: 25, y: 0, width: 20, height: splitViewHeight },
                    { x: 25 + 20, y: 0, width: 100, height: splitViewHeight },
                    { x: 25 + 100 + 20, y: 0, width: 20, height: splitViewHeight },
                    { x: 25 + 100 + 20 * 2, y: 0, width: splitViewWidth - 25 - 100 - 20 * 2,
                        height: splitViewHeight }
                ]
            },
            {
                // Same as above except vertical.
                tag: "growHandleHeight",
                orientation: Qt.Vertical,
                handleComponent: largerHandle,
                expectedGeometries: [
                    { x: 0, y: 0, width: splitViewWidth, height: 25 },
                    { x: 0, y: 25, width: splitViewWidth, height: 20 },
                    { x: 0, y: 25 + 20, width: splitViewWidth, height: 100 },
                    { x: 0, y: 25 + 100 + 20, width: splitViewWidth, height: 20 },
                    { x: 0, y: 25 + 100 + 20 * 2, width: splitViewWidth,
                        height: splitViewHeight - 25 - 100 - 20 * 2 }
                ]
            },
            {
                tag: "shrinkHandleWidth",
                orientation: Qt.Horizontal,
                handleComponent: smallerHandle,
                expectedGeometries: [
                    { x: 0, y: 0, width: 25, height: splitViewHeight },
                    { x: 25, y: 0, width: 5, height: splitViewHeight },
                    { x: 25 + 5, y: 0, width: 100, height: splitViewHeight },
                    { x: 25 + 100 + 5, y: 0, width: 5, height: splitViewHeight },
                    { x: 25 + 100 + 5 * 2, y: 0, width: splitViewWidth - 25 - 100 - 5 * 2,
                        height: splitViewHeight }
                ]
            },
            {
                // Same as above except vertical.
                tag: "shrinkHandleHeight",
                orientation: Qt.Vertical,
                handleComponent: smallerHandle,
                expectedGeometries: [
                    { x: 0, y: 0, width: splitViewWidth, height: 25 },
                    { x: 0, y: 25, width: splitViewWidth, height: 5 },
                    { x: 0, y: 25 + 5, width: splitViewWidth, height: 100 },
                    { x: 0, y: 25 + 100 + 5, width: splitViewWidth, height: 5 },
                    { x: 0, y: 25 + 100 + 5 * 2, width: splitViewWidth,
                        height: splitViewHeight - 25 - 100 - 5 * 2 }
                ]
            }
        ]
        return data
    }

    function test_handleChanges(data) {
        var control = createTemporaryObject(threeSizedItemsComponent, testCase,
            { "orientation": data.orientation })
        verify(control)

        control.handle = data.handleComponent
        verify(isPolishScheduled(control))
        verify(waitForItemPolished(control))
        compareSizes(control, data.expectedGeometries)
    }

    function test_insertRemoveItems_data() {
        var splitViewWidth = testCase.width
        var splitViewHeight = testCase.height

        var data = [
            {
                tag: "insertItemAtHorizontalEnd",
                orientation: Qt.Horizontal,
                insertItemAtIndex: 3,
                expectedGeometries: [
                    { x: 0, y: 0, width: 25, height: splitViewHeight },
                    { x: 25, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: 25 + defaultHorizontalHandleWidth, y: 0, width: 100, height: splitViewHeight },
                    { x: 25 + 100 + defaultHorizontalHandleWidth, y: 0,
                        width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    // This was the fill item originally, but since no fill item is explicitly
                    // specified, and we added an item to the right of it, it is no longer the fill item
                    // because it's no longer last.
                    { x: 25 + 100 + defaultHorizontalHandleWidth * 2, y: 0,
                        width: 200, height: splitViewHeight },
                    // Handle for newly added item.
                    { x: 25 + 100 + 200 + defaultHorizontalHandleWidth * 2, y: 0,
                        width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    // Newly added item.
                    { x: 25 + 100 + 200 + defaultHorizontalHandleWidth * 3, y: 0,
                        width: splitViewWidth - 25 - 100 - 200 - defaultHorizontalHandleWidth * 3,
                        height: splitViewHeight },
                ]
            },
            {
                tag: "insertItemAtHorizontalBeginning",
                orientation: Qt.Horizontal,
                insertItemAtIndex: 0,
                expectedGeometries: [
                    // Newly added item.
                    { x: 0, y: 0, width: 25, height: splitViewHeight },
                    { x: 25, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: 25 + defaultHorizontalHandleWidth, y: 0, width: 25, height: splitViewHeight },
                    { x: 25 * 2 + defaultHorizontalHandleWidth, y: 0,
                        width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: 25 * 2 + defaultHorizontalHandleWidth * 2, y: 0, width: 100, height: splitViewHeight },
                    { x: 25 * 2 + 100 + defaultHorizontalHandleWidth * 2, y: 0,
                        width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    // Fill item doesn't change.
                    { x: 25 * 2 + 100 + defaultHorizontalHandleWidth * 3, y: 0,
                        width: splitViewWidth - 25 * 2 - 100 - defaultHorizontalHandleWidth * 3,
                        height: splitViewHeight },
                ]
            },
            {
                tag: "removeItemFromHorizontalEnd",
                orientation: Qt.Horizontal,
                removeItemAtIndex: 2,
                expectedGeometries: [
                    { x: 0, y: 0, width: 25, height: splitViewHeight },
                    { x: 25, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: 25 + defaultHorizontalHandleWidth, y: 0,
                        width: splitViewWidth - 25 - defaultHorizontalHandleWidth, height: splitViewHeight },
                ]
            },
            {
                tag: "removeItemFromHorizontalBeginning",
                orientation: Qt.Horizontal,
                removeItemAtIndex: 0,
                expectedGeometries: [
                    { x: 0, y: 0, width: 100, height: splitViewHeight },
                    { x: 100, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: 100 + defaultHorizontalHandleWidth, y: 0,
                        width: splitViewWidth - 100 - defaultHorizontalHandleWidth, height: splitViewHeight },
                ]
            }
        ]
        return data
    }

    Component {
        id: smallRectComponent

        Rectangle {
            objectName: "darkseagreen"
            color: objectName
            implicitWidth: 25
            implicitHeight: 25
        }
    }

    function test_insertRemoveItems(data) {
        var control = createTemporaryObject(threeSizedItemsComponent, testCase,
            { "orientation": data.orientation })
        verify(control)

        if (data.hasOwnProperty("removeItemAtIndex")) {
            var itemToRemove = control.itemAt(data.removeItemAtIndex)
            verify(itemToRemove)

            control.removeItem(itemToRemove)
        } else if (data.hasOwnProperty("insertItemAtIndex")) {
            var itemToAdd = smallRectComponent.createObject(control)
            control.insertItem(data.insertItemAtIndex, itemToAdd)
        }

        verify(isPolishScheduled(control))
        verify(waitForItemPolished(control))
        compareSizes(control, data.expectedGeometries)
    }

    function test_removeAllItems() {
        var control = createTemporaryObject(threeSizedItemsComponent, testCase)
        verify(control)

        while (control.count > 0)
            var itemToRemove = control.removeItem(0)
        // Shouldn't crash.
    }

    function test_hideItems_data() {
        var splitViewWidth = testCase.width
        var splitViewHeight = testCase.height

        var data = [
            {
                tag: "hideItemAtHorizontalEnd",
                orientation: Qt.Horizontal,
                hideIndices: [2],
                expectedGeometries: [
                    { x: 0, y: 0, width: 25, height: splitViewHeight },
                    { x: 25, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: 25 + defaultHorizontalHandleWidth, y: 0,
                        width: splitViewWidth - 25 - defaultHorizontalHandleWidth, height: splitViewHeight },
                    { hidden: true }, // Handle for second item should be hidden.
                    { hidden: true } // Last item should be hidden.
                ]
            },
            {
                tag: "hideItemAtHorizontalBeginning",
                orientation: Qt.Horizontal,
                hideIndices: [0],
                expectedGeometries: [
                    { hidden: true }, // First item should be hidden.
                    { hidden: true }, // Handle for first item should be hidden.
                    { x: 0, y: 0, width: 100, height: splitViewHeight },
                    { x: 100, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: 100 + defaultHorizontalHandleWidth, y: 0,
                        width: splitViewWidth - 100 - defaultHorizontalHandleWidth, height: splitViewHeight }
                ]
            },
            {
                tag: "hideItemAtVerticalEnd",
                orientation: Qt.Vertical,
                hideIndices: [2],
                expectedGeometries: [
                    { x: 0, y: 0, width: splitViewWidth, height: 25 },
                    { x: 0, y: 25, width: splitViewWidth, height: defaultVerticalHandleHeight },
                    { x: 0, y: 25 + defaultVerticalHandleHeight,
                        width: splitViewWidth, height: splitViewHeight - 25 - defaultVerticalHandleHeight },
                    { hidden: true }, // Handle for second item should be hidden.
                    { hidden: true } // Last item should be hidden.
                ]
            },
            {
                tag: "hideItemAtVerticalBeginning",
                orientation: Qt.Vertical,
                hideIndices: [0],
                expectedGeometries: [
                    { hidden: true }, // First item should be hidden.
                    { hidden: true }, // Handle for first item should be hidden.
                    { x: 0, y: 0, width: splitViewWidth, height: 100 },
                    { x: 0, y: 100, width: splitViewWidth, height: defaultVerticalHandleHeight },
                    { x: 0, y: 100 + defaultVerticalHandleHeight,
                        width: splitViewWidth, height: splitViewHeight - 100 - defaultVerticalHandleHeight }
                ]
            },
            {
                // No handles should be visible when there's only one item.
                tag: "hideLastTwoHorizontalItems",
                orientation: Qt.Horizontal,
                hideIndices: [1, 2],
                expectedGeometries: [
                    { x: 0, y: 0, width: splitViewWidth, height: splitViewHeight },
                    { hidden: true }, // Handle for first item should be hidden.
                    { hidden: true }, // Second item should be hidden.
                    { hidden: true }, // Handle for second item should be hidden.
                    { hidden: true } // Third item should be hidden.
                ]
            }
        ]
        return data
    }

    function test_hideItems(data) {
        var control = createTemporaryObject(threeSizedItemsComponent, testCase,
            { "orientation": data.orientation })
        verify(control)

        for (var i = 0; i < data.hideIndices.length; ++i) {
            var itemToHide = control.itemAt(data.hideIndices[i])
            verify(itemToHide)
            itemToHide.visible = false
        }

        verify(isPolishScheduled(control))
        verify(waitForItemPolished(control))
        compareSizes(control, data.expectedGeometries)
    }

    function test_hideAndShowItems_data() {
        var splitViewWidth = testCase.width
        var splitViewHeight = testCase.height

        var data = [
            {
                tag: "hideLastTwoHorizontalItems",
                orientation: Qt.Horizontal,
                hideIndices: [1, 2],
                expectedGeometriesAfterHiding: [
                    { x: 0, y: 0, width: splitViewWidth, height: splitViewHeight },
                    { hidden: true }, // Handle for first item should be hidden.
                    { hidden: true }, // Second item should be hidden.
                    { hidden: true }, // Handle for second item should be hidden.
                    { hidden: true } // Third item should be hidden.
                ],
                showIndices: [1],
                expectedGeometriesAfterShowing: [
                    // First item should be visible with its implicit size.
                    { x: 0, y: 0, width: 25, height: splitViewHeight },
                    // Handle for first item should be visible.
                    { x: 25, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    // Second item should be visible and fill.
                    { x: 25 + defaultHorizontalHandleWidth, y: 0,
                        width: splitViewWidth - 25 - defaultHorizontalHandleWidth, height: splitViewHeight },
                    { hidden: true }, // Handle for second item should be hidden.
                    { hidden: true } // Third item should be hidden.
                ]
            }
        ]
        return data
    }

    function test_hideAndShowItems(data) {
        var control = createTemporaryObject(threeSizedItemsComponent, testCase,
            { "orientation": data.orientation })
        verify(control)

        for (var i = 0; i < data.hideIndices.length; ++i) {
            var itemToHide = control.itemAt(data.hideIndices[i])
            verify(itemToHide)
            itemToHide.visible = false
        }
        verify(isPolishScheduled(control))
        verify(waitForItemPolished(control))
        compareSizes(control, data.expectedGeometriesAfterHiding, "after hiding")

        for (i = 0; i < data.showIndices.length; ++i) {
            var itemToShow = control.itemAt(data.showIndices[i])
            verify(itemToShow)
            itemToShow.visible = true
        }
        verify(isPolishScheduled(control))
        verify(waitForItemPolished(control))
        compareSizes(control, data.expectedGeometriesAfterShowing, "after showing")
    }

    function test_moveHiddenItems_data() {
        var splitViewWidth = testCase.width
        var splitViewHeight = testCase.height

        var data = [
            {
                tag: "hideSecondItemAndMoveItToFirst",
                orientation: Qt.Horizontal,
                hideIndices: [1],
                moveFromIndex: 1,
                moveToIndex: 0,
                expectedGeometriesAfterMoving: [
                    { hidden: true }, // First item (was second) should be hidden.
                    { hidden: true }, // Handle for first item should be hidden.
                    // Second item (was first) should get its implicit size.
                    { x: 0, y: 0, width: 25, height: splitViewHeight },
                    { x: 25, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: 25 + defaultHorizontalHandleWidth, y: 0,
                        width: splitViewWidth - 25 - defaultHorizontalHandleWidth, height: splitViewHeight },
                ],
                showIndices: [0],
                expectedGeometriesAfterShowing: [
                    // First item (was second) should be visible with its implicit size.
                    { x: 0, y: 0, width: 100, height: splitViewHeight },
                    // Handle for first item (was second) should be visible.
                    { x: 100, y: 0, width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    // Second item (was first) should be visible with its implicit size.
                    { x: 100 + defaultHorizontalHandleWidth, y: 0,
                        width: 25, height: splitViewHeight },
                    { x: 100 + 25 + defaultHorizontalHandleWidth, y: 0,
                        width: defaultHorizontalHandleWidth, height: splitViewHeight },
                    { x: 100 + 25 + defaultHorizontalHandleWidth * 2, y: 0,
                        width: splitViewWidth - 100 - 25 - defaultHorizontalHandleWidth * 2, height: splitViewHeight }
                ]
            }
        ]
        return data
    }

    function test_moveHiddenItems(data) {
        var control = createTemporaryObject(threeSizedItemsComponent, testCase,
            { "orientation": data.orientation })
        verify(control)

        for (var i = 0; i < data.hideIndices.length; ++i) {
            var itemToHide = control.itemAt(data.hideIndices[i])
            verify(itemToHide)
            itemToHide.visible = false
        }

        control.moveItem(data.moveFromIndex, data.moveToIndex)
        verify(isPolishScheduled(control))
        verify(waitForItemPolished(control))
        compareSizes(control, data.expectedGeometriesAfterMoving, "after moving")

        for (i = 0; i < data.showIndices.length; ++i) {
            var itemToShow = control.itemAt(data.showIndices[i])
            verify(itemToShow)
            itemToShow.visible = true
        }
        verify(isPolishScheduled(control))
        verify(waitForItemPolished(control))
        compareSizes(control, data.expectedGeometriesAfterShowing, "after showing")
    }

    Component {
        id: flickableComponent

        Flickable {
            anchors.fill: parent
            anchors.margins: 100
        }
    }

    function test_draggingHandleInFlickable() {
        var flickable = createTemporaryObject(flickableComponent, testCase)
        verify(flickable)

        var control = threeSizedItemsComponent.createObject(flickable.contentItem,
            { "orientation": data.orientation })
        verify(control)

        control.anchors.fill = undefined
        control.width = 400
        control.height = control.parent.height - 100
        flickable.contentWidth = control.width
        flickable.contentHeight = control.height
        verify(isPolishScheduled(control))
        verify(waitForItemPolished(control))

        var contentXSpy = signalSpyComponent.createObject(flickable,
            { target: flickable, signalName: "contentXChanged" })
        verify(contentXSpy.valid)
        var contentYSpy = signalSpyComponent.createObject(flickable,
            { target: flickable, signalName: "contentYChanged" })
        verify(contentYSpy.valid)

        // Drag the first handle to the right;
        // the flickable's contentX and contentY shouldn't change.
        var firstItem = control.itemAt(0)
        var firstItemOriginalWidth = firstItem.width
        var handles = findHandles(control)
        var firstHandle = handles[0]
        // Add some vertical movement in there as well.
        mouseDrag(firstHandle, firstHandle.width / 2, firstHandle.height / 2, 100, 50)
        compare(contentXSpy.count, 0)
        compare(contentYSpy.count, 0)
        verify(firstItem.width > firstItemOriginalWidth)

        // Now do the same except vertically.
        control.orientation = Qt.Vertical
        control.width = control.parent.width - 100
        control.height = 400
        var firstItemOriginalHeight = firstItem.height
        verify(isPolishScheduled(control))
        verify(waitForItemPolished(control))

        // Add some horizontal movement in there as well.
        mouseDrag(firstHandle, firstHandle.width / 2, firstHandle.height / 2, 50, 100)
        compare(contentXSpy.count, 0)
        compare(contentYSpy.count, 0)
        verify(firstItem.height > firstItemOriginalHeight)
    }

    function test_hoveredPressed() {
        if ((Qt.platform.pluginName === "offscreen") || (Qt.platform.pluginName === "minimal"))
            skip("Mouse hovering not functional on offscreen/minimal platforms")

        var control = createTemporaryObject(threeSizedItemsComponent, testCase)
        verify(control)
        control.anchors.margins = 50

        var handles = findHandles(control)
        var firstHandle = handles[0]

        var handleCenter = control.mapFromItem(firstHandle, firstHandle.width / 2, firstHandle.height / 2)
        // Test fails if we don't do two moves for some reason...
        mouseMove(control, handleCenter.x, handleCenter.y)
        mouseMove(control, handleCenter.x, handleCenter.y)
        verify(firstHandle.SplitHandle.hovered)
        verify(!firstHandle.SplitHandle.pressed)

        mousePress(control, handleCenter.x, handleCenter.y)
        verify(firstHandle.SplitHandle.hovered)
        verify(firstHandle.SplitHandle.pressed)

        mouseRelease(control, handleCenter.x, handleCenter.y)
        verify(firstHandle.SplitHandle.hovered)
        verify(!firstHandle.SplitHandle.pressed)

        mouseMove(control, 0, 0)
        verify(!firstHandle.SplitHandle.hovered)
        verify(!firstHandle.SplitHandle.pressed)
    }

    // Tests removing/adding/moving an item while it's pressed.
    function test_modifyWhileHoveredPressed() {
        if ((Qt.platform.pluginName === "offscreen") || (Qt.platform.pluginName === "minimal"))
            skip("Mouse hovering not functional on offscreen/minimal platforms")

        var control = createTemporaryObject(threeSizedItemsComponent, testCase)
        verify(control)
        control.anchors.margins = 50

        var handles = findHandles(control)
        var firstHandle = handles[0]

        // First, ensure that the handle is hovered + pressed.
        var handleCenter = control.mapFromItem(firstHandle, firstHandle.width / 2, firstHandle.height / 2)
        // Test fails if we don't do two moves for some reason...
        mouseMove(control, handleCenter.x, handleCenter.y)
        mouseMove(control, handleCenter.x, handleCenter.y)
        verify(firstHandle.SplitHandle.hovered)
        verify(!firstHandle.SplitHandle.pressed)

        mousePress(control, handleCenter.x, handleCenter.y)
        verify(firstHandle.SplitHandle.hovered)
        verify(firstHandle.SplitHandle.pressed)

        // Then, remove it by removing the first item.
        control.removeItem(0)
        handles = findHandles(control)
        firstHandle = null
        compare(handles.length, 1)

        // No handles should be hovered/pressed.
        for (var i = 0; i < handles.length; ++i) {
            var handle = handles[i]
            verify(!handle.SplitHandle.hovered, "handle at index " + i + " should not be hovered")
            verify(!handle.SplitHandle.pressed, "handle at index " + i + " should not be hovered")
        }

        mouseRelease(control, handleCenter.x, handleCenter.y)
    }

    Component {
        id: settingsComponent
        Settings {
            id: settings
        }
    }

    function test_saveAndRestoreState_data() {
        return [
            { tag: "Horizontal", orientation: Qt.Horizontal, propertyName: "preferredWidth", propertyValue: 123 },
            { tag: "Vertical", orientation: Qt.Vertical, propertyName: "preferredHeight", propertyValue: 234 }
        ]
    }

    function test_saveAndRestoreState(data) {
        var control = createTemporaryObject(threeSizedItemsComponent, testCase, { orientation: data.orientation })
        verify(control)
        compare(control.orientation, data.orientation)

        var lastItem = control.itemAt(2)
        verify(lastItem)
        lastItem.SplitView[data.propertyName] = data.propertyValue

        // Save the state.
        var settings = createTemporaryObject(settingsComponent, testCase)
        verify(settings)
        settings.setValue("splitView", control.saveState())

        // Recreate the item to restore it to its "default" values.
        control = createTemporaryObject(threeSizedItemsComponent, testCase)
        lastItem = control.itemAt(2)
        verify(lastItem)
        compare(lastItem.SplitView[data.propertyName], -1)

        settings = createTemporaryObject(settingsComponent, testCase)
        verify(settings)

        // Restore the state.
        control.restoreState(settings.value("splitView"))
        compare(lastItem.SplitView[data.propertyName], data.propertyValue)
    }

    function test_changePreferredSizeDuringLayout() {
        var control = createTemporaryObject(threeSizedItemsComponent, testCase)
        verify(control)

        var firstItem = control.itemAt(0)
        var secondItem = control.itemAt(1)
        secondItem.widthChanged.connect(function() {
            if (secondItem.width < 10)
                firstItem.SplitView.preferredWidth = 50
        })

        // Change the size of the item so that a layout happens, but
        // make the size small enough that the item's onWidthChanged handler gets triggered.
        // The onWidthChanged handler will set the preferredWidth of another item during the
        // layout, so we need to make sure the assignment isn't lost since we return early in that case.
        secondItem.implicitWidth = 5
        verify(isPolishScheduled(control))
        verify(waitForItemPolished(control))
        compare(secondItem.width, 5)
        compare(firstItem.width, 50)

        // Now do the same for height.
        control.orientation = Qt.Vertical
        secondItem.heightChanged.connect(function() {
            if (secondItem.height < 10)
                firstItem.SplitView.preferredHeight = 50
        })
        // Get the polishes for the orientation out of the way so that they
        // don't intefere with our results.
        verify(isPolishScheduled(control))
        verify(waitForItemPolished(control))

        secondItem.implicitHeight = 5
        verify(isPolishScheduled(control))
        verify(waitForItemPolished(control))
        compare(secondItem.height, 5)
        compare(firstItem.height, 50)
    }

    // When the user drags a handle, we internally set preferredWidth/Height
    // to reflect the new value. However, we also have to make sure that when
    // we do so, it doesn't trigger a delayed layout. This is why we have
    // m_ignoreNextDelayedLayoutRequest. This test checks that
    // m_ignoreNextDelayedLayoutRequest doesn't interfere with any action from
    // the user that results in a delayed layout.
    function test_changePreferredSizeDuringLayoutWhileDraggingHandle() {
        var control = createTemporaryObject(threeSizedItemsComponent, testCase)
        verify(control)

        var firstItem = control.itemAt(0)
        var secondItem = control.itemAt(1)
        firstItem.widthChanged.connect(function() {
            if (firstItem.width === 0)
                secondItem.SplitView.preferredWidth = 50
        })

        // Start dragging the handle.
        var handles = findHandles(control)
        var targetHandle = handles[0]
        mousePress(targetHandle)
        verify(control.resizing)
        var localPos = testCase.mapToItem(targetHandle, 15, testCase.height / 2)

        // Move the handle to the very left, so that the item's width becomes zero.
        mouseMove(targetHandle, -100, targetHandle.height / 2)
        verify(control.resizing)
        compare(firstItem.width, 0)
        compare(secondItem.SplitView.preferredWidth, 50)
        compare(secondItem.width, 50)
        mouseRelease(targetHandle, -100, targetHandle.height / 2, Qt.LeftButton)
        verify(!control.resizing)
    }

    Component {
        id: oneItemComponent

        SplitView {
            Item {}
        }
    }

    // QTBUG-79270
    function test_hideSplitViewWithOneItem() {
        var control = createTemporaryObject(oneItemComponent, testCase)
        verify(control)
        // Shouldn't be an assertion failure.
        control.visible = false
    }

    // QTBUG-79302: ensure that the Repeater's items are actually generated.
    // test_dragHandle:repeater tests dragging behavior with a Repeater.
    function test_repeater(data) {
        var control = createTemporaryObject(repeaterSplitViewComponent, testCase)
        verify(control)
        compare(control.repeater.count, 3)
        compare(control.contentChildren.length, 3)
    }

    Component {
        id: hoverableChildrenSplitViewComponent

        SplitView {
            handle: handleComponent
            anchors.fill: parent

            MouseArea {
                objectName: "mouseArea1"
                hoverEnabled: true

                SplitView.preferredWidth: 200
            }
            MouseArea {
                objectName: "mouseArea2"
                hoverEnabled: true
            }
        }
    }

    function test_hoverableChilden() {
        if (Qt.platform.pluginName === "offscreen" || Qt.platform.pluginName === "minimal")
            skip("Mouse hovering not functional on offscreen/minimal platforms")

        var control = createTemporaryObject(hoverableChildrenSplitViewComponent, testCase)
        verify(control)

        verify(isPolishScheduled(control))
        verify(waitForItemPolished(control))

        // Move the mouse over the handle.
        var handles = findHandles(control)
        var targetHandle = handles[0]
        // Test fails if we don't do two moves for some reason...
        mouseMove(targetHandle, targetHandle.width / 2, targetHandle.height / 2)
        mouseMove(targetHandle, targetHandle.width / 2, targetHandle.height / 2)
        verify(targetHandle.SplitHandle.hovered)

        // Move the mouse to the MouseArea on the left. The handle should no longer be hovered.
        mouseMove(control, 100, control.height / 2)
        verify(!targetHandle.SplitHandle.hovered)

        // Move the mouse back over the handle.
        mouseMove(targetHandle, targetHandle.width / 2, targetHandle.height / 2)
        verify(targetHandle.SplitHandle.hovered)

        // Move the mouse to the MouseArea on the right. The handle should no longer be hovered.
        mouseMove(control, control.width - 100, control.height / 2)
        verify(!targetHandle.SplitHandle.hovered)
    }
}
