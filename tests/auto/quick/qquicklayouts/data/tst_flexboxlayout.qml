// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest
import QtQuick.Controls
import QtQuick.Layouts
import "LayoutHelperLibrary.js" as LayoutHelpers

import org.qtproject.Test

Item {
    id: container
    width: 200
    height: 200
    TestCase {
        id: testCase
        name: "Tests_FlexboxLayout"
        when: windowShown
        width: 200
        height: 200

        Component {
            id: itemsWithAnchorsLayout_Component
            FlexboxLayout {
                wrap: FlexboxLayout.Wrap
                Item {
                    anchors.fill: parent
                    implicitWidth: 10
                    implicitHeight: 10
                }
                Item {
                    anchors.centerIn: parent
                    implicitWidth: 10
                    implicitHeight: 10
                }
                Item {
                    anchors.left: parent.left
                    implicitWidth: 10
                    implicitHeight: 10
                }
                Item {
                    anchors.right: parent.right
                    implicitWidth: 10
                    implicitHeight: 10
                }
                Item {
                    anchors.top: parent.top
                    implicitWidth: 10
                    implicitHeight: 10
                }
                Item {
                    anchors.bottom: parent.bottom
                    implicitWidth: 10
                    implicitHeight: 10
                }
                Item {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    implicitWidth: 10
                    implicitHeight: 10
                }
                Item {
                    anchors.margins: 42     // although silly, it should not cause a warning from the Layouts POV
                    implicitWidth: 10
                    implicitHeight: 10
                }
            }
        }

        function test_warnAboutLayoutItemsWithAnchors()
        {
            var regex = new RegExp(".*: Detected anchors on an item that is managed by a layout. "
                                 + "This is undefined behavior; use FlexboxLayout alignment properties instead.")
            for (var i = 0; i < 7; ++i) {
                ignoreWarning(regex)
            }
            var layout = itemsWithAnchorsLayout_Component.createObject(container)
            waitForRendering(layout)
            layout.destroy()
        }

        Component {
            id: flexItem
            Rectangle {
                implicitWidth: 20
                implicitHeight: 20
            }
        }

        Component {
            id: dynamicFlexboxLayoutComponent
            FlexboxLayout { }
        }

        function test_addAndRemoveItems() {
            let flexboxLayout = createTemporaryObject(dynamicFlexboxLayoutComponent, container)
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.implicitWidth, 0)
            compare(flexboxLayout.implicitHeight, 0)

            let rectItem1 = flexItem.createObject(flexboxLayout)
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.implicitWidth, 20)
            compare(flexboxLayout.implicitHeight, 20)

            let rectItem2 = flexItem.createObject(flexboxLayout)
            rectItem2.Layout.preferredWidth = 30
            rectItem2.Layout.preferredHeight = 30
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.implicitWidth, 50)
            compare(flexboxLayout.implicitHeight, 30)

            // Remove the second rectangle
            rectItem2.destroy()
            wait(0)
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.implicitWidth, 20)
            compare(flexboxLayout.implicitHeight, 20)

            // Remove the first rectangle
            rectItem1.destroy()
            wait(0)
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.implicitWidth, 0)
            compare(flexboxLayout.implicitHeight, 0)
        }

        Component {
            id: staticFlexboxLayoutComponent
            FlexboxLayout {
                Rectangle {
                    implicitWidth: 20
                    implicitHeight: 20
                }
                Rectangle {
                    Layout.preferredWidth: 30
                    Layout.preferredHeight: 30
                }
            }
        }

        function test_staticItems() {
            let flexboxLayout = createTemporaryObject(staticFlexboxLayoutComponent, container)
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.implicitWidth, 50)
            compare(flexboxLayout.implicitHeight, 30)
            flexboxLayout.destroy()
        }

        function test_flexItemDirection() {
            let flexboxLayout = createTemporaryObject(dynamicFlexboxLayoutComponent, container)
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.implicitWidth, 0)
            compare(flexboxLayout.implicitHeight, 0)

            compare (flexboxLayout.direction, FlexboxLayout.Row)

            let rectItem1 = flexItem.createObject(flexboxLayout)
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.implicitWidth, 20)
            compare(flexboxLayout.implicitHeight, 20)

            let rectItem2 = flexItem.createObject(flexboxLayout)
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.implicitWidth, 40)
            compare(flexboxLayout.implicitHeight, 20)

            compare(rectItem1.x, 0)
            compare(rectItem1.y, 0)
            compare(rectItem2.x, rectItem1.implicitWidth)
            compare(rectItem2.y, 0)

            // Set the flexbox direction to FlexboxLayout.RowReverse
            flexboxLayout.direction = FlexboxLayout.RowReverse
            compare (flexboxLayout.direction, FlexboxLayout.RowReverse)
            waitForItemPolished(flexboxLayout)
            compare(rectItem1.x, rectItem2.implicitWidth)
            compare(rectItem1.y, 0)
            compare(rectItem2.x, 0)
            compare(rectItem2.y, 0)

            // Set the flexbox direction to FlexboxLayout.Column
            flexboxLayout.direction = FlexboxLayout.Column
            compare (flexboxLayout.direction, FlexboxLayout.Column)
            waitForItemPolished(flexboxLayout)
            compare(rectItem1.x, 0)
            compare(rectItem1.y, 0)
            compare(rectItem2.x, 0)
            compare(rectItem2.y, rectItem1.implicitHeight)

            // Set the flexbox direction to FlexboxLayout.ColumnReverse
            flexboxLayout.direction = FlexboxLayout.ColumnReverse
            compare (flexboxLayout.direction, FlexboxLayout.ColumnReverse)
            waitForItemPolished(flexboxLayout)
            compare(rectItem1.x, 0)
            compare(rectItem1.y, rectItem2.implicitHeight)
            compare(rectItem2.x, 0)
            compare(rectItem2.y, 0)

            rectItem1.destroy()
            rectItem2.destroy()
            flexboxLayout.destroy()
        }

        function test_flexItemWrap() {
            let flexboxLayout = createTemporaryObject(dynamicFlexboxLayoutComponent, container)
            flexboxLayout.width = 40
            flexboxLayout.height = 40
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.implicitWidth, 0)
            compare(flexboxLayout.implicitHeight, 0)
            compare(flexboxLayout.width, 40)
            compare(flexboxLayout.height, 40)

            // direction: FlexboxLayout.Row (default), wrap: FlexboxLayout.NoWrap
            compare (flexboxLayout.wrap, FlexboxLayout.NoWrap)

            let rectItem1 = flexItem.createObject(flexboxLayout)
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.implicitWidth, 20)
            compare(flexboxLayout.implicitHeight, 20)

            let rectItem2 = flexItem.createObject(flexboxLayout)
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.implicitWidth, 40)
            compare(flexboxLayout.implicitHeight, 20)

            compare(rectItem1.x, 0)
            compare(rectItem1.y, 0)
            compare(rectItem2.x, rectItem1.implicitWidth)
            compare(rectItem2.y, 0)

            // direction: FlexboxLayout.Row (default), wrap: FlexboxLayout.Wrap
            flexboxLayout.wrap = FlexboxLayout.Wrap
            flexboxLayout.width = 30
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.width, 30)
            compare(flexboxLayout.height, 40)
            compare(rectItem1.x, 0)
            compare(rectItem1.y, 0)
            compare(rectItem2.x, 0)
            compare(rectItem2.y, rectItem1.implicitHeight)

            // direction: FlexboxLayout.Row (default), wrap: FlexboxLayout.WrapReverse
            flexboxLayout.wrap = FlexboxLayout.WrapReverse
            flexboxLayout.width = 30
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.width, 30)
            compare(flexboxLayout.height, 40)
            compare(rectItem1.x, 0)
            compare(rectItem1.y, rectItem2.implicitHeight)
            compare(rectItem2.x, 0)
            compare(rectItem2.y, 0)

            rectItem1.destroy()
            rectItem2.destroy()
            flexboxLayout.destroy()
        }

        function itemRect(item) {
            return [item.x, item.y, item.width, item.height]
        }
        // alignItem affects flexbox layout container cross-axis
        function test_flexAlignItems() {
            let flexboxLayout = createTemporaryObject(dynamicFlexboxLayoutComponent, container)
            flexboxLayout.width = 60
            flexboxLayout.height = 40
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.width, 60)
            compare(flexboxLayout.height, 40)

            // direction: FlexboxLayout.Row (default)
            // wrap: FlexboxLayout.NoWrap
            compare(flexboxLayout.direction, FlexboxLayout.Row)
            compare(flexboxLayout.wrap, FlexboxLayout.NoWrap)

            let rectItem1 = flexItem.createObject(flexboxLayout)
            waitForItemPolished(flexboxLayout)

            let rectItem2 = flexItem.createObject(flexboxLayout)
            rectItem2.Layout.preferredWidth = 20
            rectItem2.Layout.preferredHeight = 40
            waitForItemPolished(flexboxLayout)

            // alignItems: FlexboxLayout.AlignStart (default)
            compare(flexboxLayout.alignItems, FlexboxLayout.AlignStart)
            compare(rectItem1.x, 0)
            compare(rectItem1.y, 0)
            compare(rectItem2.x, rectItem1.implicitWidth)
            compare(rectItem2.y, 0)

            // alignItems: FlexboxLayout.AlignCenter
            flexboxLayout.alignItems = FlexboxLayout.AlignCenter
            waitForItemPolished(flexboxLayout)
            compare(rectItem1.x, 0)
            compare(rectItem1.y, 10)
            compare(rectItem2.x, rectItem1.implicitWidth)
            compare(rectItem2.y, 0)

            // alignItems: FlexboxLayout.AlignEnd
            flexboxLayout.alignItems = FlexboxLayout.AlignEnd
            waitForItemPolished(flexboxLayout)
            compare(rectItem1.x, 0)
            compare(rectItem1.y, 20)
            compare(rectItem2.x, rectItem1.implicitWidth)
            compare(rectItem2.y, 0)

            // Wrap the items and check whether alignItems has any effect
            flexboxLayout.wrap = FlexboxLayout.Wrap
            flexboxLayout.alignItems = FlexboxLayout.AlignStart
            rectItem2.Layout.preferredWidth = 50
            waitForItemPolished(flexboxLayout)
            compare(rectItem1.x, 0)
            compare(rectItem1.y, 0)
            compare(rectItem2.x, 0)
            compare(rectItem2.y, rectItem1.implicitHeight)

            flexboxLayout.alignItems = FlexboxLayout.AlignCenter
            waitForItemPolished(flexboxLayout)
            compare(rectItem1.x, 0)
            compare(rectItem1.y, 0)
            compare(rectItem2.x, 0)
            compare(rectItem2.y, rectItem1.implicitHeight)

            flexboxLayout.alignItems = FlexboxLayout.AlignEnd
            waitForItemPolished(flexboxLayout)
            compare(rectItem1.x, 0)
            compare(rectItem1.y, 0)
            compare(rectItem2.x, 0)
            compare(rectItem2.y, rectItem1.implicitHeight)

            // QTBUG-142021: item with fillX misaligns along the primary axis
            flexboxLayout.alignItems = FlexboxLayout.AlignCenter
            flexboxLayout.wrap = FlexboxLayout.NoWrap
            rectItem1.Layout.fillHeight = true
            rectItem2.Layout.preferredWidth = 10
            rectItem2.Layout.preferredHeight = 30
            rectItem2.Layout.fillWidth = true
            waitForItemPolished(flexboxLayout)
            compare(itemRect(rectItem1), [ 0,  0, 20, 40])
            compare(itemRect(rectItem2), [20,  5, 40, 30])

            rectItem1.destroy()
            rectItem2.destroy()
            flexboxLayout.destroy()
        }

        // alignContent affects flexbox layout container cross-axis
        function test_flexAlignContent() {
            let flexboxLayout = createTemporaryObject(dynamicFlexboxLayoutComponent, container)
            flexboxLayout.width = 40
            flexboxLayout.height = 100
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.width, 40)
            compare(flexboxLayout.height, 100)

            // direction: FlexboxLayout.Row (default)
            // wrap: FlexboxLayout.Wrap
            compare(flexboxLayout.direction, FlexboxLayout.Row)
            flexboxLayout.wrap = FlexboxLayout.Wrap
            compare(flexboxLayout.wrap, FlexboxLayout.Wrap)

            let rectItem1 = flexItem.createObject(flexboxLayout)
            waitForItemPolished(flexboxLayout)

            let rectItem2 = flexItem.createObject(flexboxLayout)
            rectItem2.Layout.preferredWidth = 20
            rectItem2.Layout.preferredHeight = 40
            waitForItemPolished(flexboxLayout)

            let rectItem3 = flexItem.createObject(flexboxLayout)
            rectItem3.Layout.preferredWidth = 30
            rectItem3.Layout.preferredHeight = 40
            waitForItemPolished(flexboxLayout)

            // alignContent: FlexboxLayout.AlignStart
            compare(flexboxLayout.alignContent, FlexboxLayout.AlignStart)
            compare(rectItem1.x, 0)
            compare(rectItem1.y, 0)
            compare(rectItem2.x, rectItem1.implicitWidth)
            compare(rectItem2.y, 0)
            compare(rectItem3.x, 0)
            compare(rectItem3.y, Math.max(rectItem1.height, rectItem2.height))

            // alignContent: FlexboxLayout.AlignCenter
            flexboxLayout.alignContent = FlexboxLayout.AlignCenter
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.alignContent, FlexboxLayout.AlignCenter)
            const yCenterPos = flexboxLayout.height / 2 - (Math.max(rectItem1.height, rectItem2.height) + rectItem3.height) / 2
            compare(rectItem1.x, 0)
            compare(rectItem1.y, yCenterPos)
            compare(rectItem2.x, rectItem1.implicitWidth)
            compare(rectItem2.y, yCenterPos)
            compare(rectItem3.x, 0)
            compare(rectItem3.y, yCenterPos + Math.max(rectItem1.height, rectItem2.height))

            // alignContent: FlexboxLayout.AlignEnd
            flexboxLayout.alignContent = FlexboxLayout.AlignEnd
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.alignContent, FlexboxLayout.AlignEnd)
            const yEndPos = flexboxLayout.height - (Math.max(rectItem1.height, rectItem2.height) + rectItem3.height)
            compare(rectItem1.x, 0)
            compare(rectItem1.y, yEndPos)
            compare(rectItem2.x, rectItem1.implicitWidth)
            compare(rectItem2.y, yEndPos)
            compare(rectItem3.x, 0)
            compare(rectItem3.y, yEndPos + Math.max(rectItem1.height, rectItem2.height))

            // alignContent: flexboxLayout.AlignStretch
            // The size of the flex item affects the position of the item within the flex
            // container
            flexboxLayout.alignContent = FlexboxLayout.AlignStretch
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.alignContent, FlexboxLayout.AlignStretch)
            compare(rectItem1.x, 0)
            compare(rectItem1.y, 0)
            compare(rectItem2.x, rectItem1.width)
            compare(rectItem2.y, 0)
            compare(rectItem3.x, 0)
            compare(rectItem3.y, flexboxLayout.height / 2)

            // alignContent: flexboxLayout.AlignSpaceBetween
            flexboxLayout.alignContent = FlexboxLayout.AlignSpaceBetween
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.alignContent, FlexboxLayout.AlignSpaceBetween)
            compare(rectItem1.x, 0)
            compare(rectItem1.y, 0)
            compare(rectItem2.x, rectItem1.width)
            compare(rectItem2.y, 0)
            compare(rectItem3.x, 0)
            compare(rectItem3.y, flexboxLayout.height - rectItem3.height)

            // alignContent: flexboxLayout.AlignSpaceAround
            flexboxLayout.alignContent = FlexboxLayout.AlignSpaceAround
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.alignContent, FlexboxLayout.AlignSpaceAround)
            const spaceBetween = rectItem3.y - (rectItem2.y + rectItem2.height)
            compare(rectItem1.x, 0)
            compare(rectItem1.y, spaceBetween / 2)
            compare(rectItem2.x, rectItem1.width)
            compare(rectItem2.y, spaceBetween / 2)
            compare(rectItem3.x, 0)
            compare(rectItem3.y, flexboxLayout.height - rectItem3.height - spaceBetween / 2)

            // alignContent: flexboxLayout.AlignSpaceEvenly
            const regex = new RegExp(".*: Currently not supported for Flexbox layout container")
            ignoreWarning(regex)
            flexboxLayout.alignContent = FlexboxLayout.AlignSpaceEvenly
            waitForItemPolished(flexboxLayout)
            // Since its not supported, it has to be stay with the existing alignment value
            compare(flexboxLayout.alignContent, FlexboxLayout.AlignSpaceAround)

            rectItem1.destroy()
            rectItem2.destroy()
            rectItem3.destroy()
            flexboxLayout.destroy()
        }

        // justifyContent affects flexbox layout container main-axis
        function test_flexJustifyContent() {
            let flexboxLayout = createTemporaryObject(dynamicFlexboxLayoutComponent, container)
            flexboxLayout.width = 100
            flexboxLayout.height = 60
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.width, 100)
            compare(flexboxLayout.height, 60)

            // direction: FlexboxLayout.Column
            // wrap: FlexboxLayout.Wrap
            flexboxLayout.direction = FlexboxLayout.Column
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.direction, FlexboxLayout.Column)
            flexboxLayout.wrap = FlexboxLayout.Wrap
            compare(flexboxLayout.wrap, FlexboxLayout.Wrap)

            let rectItem1 = flexItem.createObject(flexboxLayout)
            waitForItemPolished(flexboxLayout)

            let rectItem2 = flexItem.createObject(flexboxLayout)
            rectItem2.Layout.preferredWidth = 40
            rectItem2.Layout.preferredHeight = 20
            waitForItemPolished(flexboxLayout)

            let rectItem3 = flexItem.createObject(flexboxLayout)
            rectItem3.Layout.preferredWidth = 40
            rectItem3.Layout.preferredHeight = 20
            waitForItemPolished(flexboxLayout)

            // justifyContent: the default value is JustifyStart
            flexboxLayout.justifyContent = FlexboxLayout.JustifyStart
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.justifyContent, FlexboxLayout.JustifyStart)
            compare(rectItem1.x, 0)
            compare(rectItem1.y, 0)
            compare(rectItem2.x, 0)
            compare(rectItem2.y, rectItem1.height)
            compare(rectItem3.x, 0)
            compare(rectItem3.y, rectItem1.height + rectItem2.height)

            // justifyContent: FlexboxLayout.JustifyCenter
            rectItem3.Layout.preferredHeight = 30
            flexboxLayout.justifyContent = FlexboxLayout.JustifyCenter
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.justifyContent, FlexboxLayout.JustifyCenter)
            compare(rectItem1.x, 0)
            compare(rectItem1.y, flexboxLayout.height / 2 - rectItem1.height)
            compare(rectItem2.x, 0)
            compare(rectItem2.y, flexboxLayout.height / 2)
            compare(rectItem3.x, Math.max(rectItem1.width, rectItem2.width))
            compare(rectItem3.y, flexboxLayout.height / 2 - rectItem3.height / 2)

            // justifyContent: FlexboxLayout.JustifyEnd
            flexboxLayout.justifyContent = FlexboxLayout.JustifyEnd
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.justifyContent, FlexboxLayout.JustifyEnd)
            compare(rectItem1.x, 0)
            compare(rectItem1.y, flexboxLayout.height - (rectItem1.height + rectItem2.height))
            compare(rectItem2.x, 0)
            compare(rectItem2.y, flexboxLayout.height - rectItem2.height)
            compare(rectItem3.x, Math.max(rectItem1.width, rectItem2.width))
            compare(rectItem3.y, flexboxLayout.height - rectItem3.height)

            // justifyContent: FlexboxLayout.JustifySpaceBetween
            flexboxLayout.justifyContent = FlexboxLayout.JustifySpaceBetween
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.justifyContent, FlexboxLayout.JustifySpaceBetween)
            compare(rectItem1.x, 0)
            compare(rectItem1.y, 0)
            compare(rectItem2.x, 0)
            compare(rectItem2.y, flexboxLayout.height - rectItem2.height)
            compare(rectItem3.x, Math.max(rectItem1.width, rectItem2.width))
            compare(rectItem3.y, 0)

            // justifyContent: FlexboxLayout.JustifySpaceAround
            flexboxLayout.justifyContent = FlexboxLayout.JustifySpaceAround
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.justifyContent, FlexboxLayout.JustifySpaceAround)
            const spaceBetween = rectItem2.y - (rectItem1.y + rectItem1.height)
            compare(rectItem1.x, 0)
            compare(rectItem1.y, spaceBetween / 2)
            compare(rectItem2.x, 0)
            compare(rectItem2.y, flexboxLayout.height - spaceBetween / 2 - rectItem2.height)
            compare(rectItem3.x, Math.max(rectItem1.width, rectItem2.width))
            compare(rectItem3.y, flexboxLayout.height / 2 - rectItem3.height / 2)

            // justifyContent: FlexboxLayout.JustifySpaceEvenly
            flexboxLayout.justifyContent = FlexboxLayout.JustifySpaceEvenly
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.justifyContent, FlexboxLayout.JustifySpaceEvenly)
            const evenSpace = flexboxLayout.height - (rectItem1.height + rectItem2.height)
            compare(rectItem1.x, 0)
            // TODO: Need to check why the ceil value been considered here in the yoga layout ???
            compare(rectItem1.y, Math.ceil(evenSpace / 3))
            compare(rectItem2.x, 0)
            // TODO: Need to check why the floor value been considered here in the yoga layout ???
            compare(rectItem2.y, Math.floor(evenSpace / 3) + rectItem1.y + rectItem1.height)
            compare(rectItem3.x, Math.max(rectItem1.width, rectItem2.width))
            compare(rectItem3.y, flexboxLayout.height / 2 - rectItem3.height / 2)

            rectItem1.destroy()
            rectItem2.destroy()
            rectItem3.destroy()
            flexboxLayout.destroy()
        }

        // alignSelf override parent layout alignItem
        function test_flexOverrideAlignItemInChild() {
            let flexboxLayout = createTemporaryObject(dynamicFlexboxLayoutComponent, container)
            flexboxLayout.width = 60
            flexboxLayout.height = 40
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.width, 60)
            compare(flexboxLayout.height, 40)

            // direction: FlexboxLayout.Row (default)
            // wrap: FlexboxLayout.Wrap
            compare(flexboxLayout.direction, FlexboxLayout.Row)
            compare(flexboxLayout.wrap, FlexboxLayout.NoWrap)

            let rectItem1 = flexItem.createObject(flexboxLayout)
            rectItem1.FlexboxLayout.alignSelf = FlexboxLayout.AlignCenter
            waitForItemPolished(flexboxLayout)

            let rectItem2 = flexItem.createObject(flexboxLayout)
            rectItem2.Layout.preferredWidth = 20
            rectItem2.Layout.preferredHeight = 40
            waitForItemPolished(flexboxLayout)

            // alignItems: FlexboxLayout.AlignStart (default)
            compare(rectItem1.FlexboxLayout.alignSelf, FlexboxLayout.AlignCenter)
            compare(rectItem2.FlexboxLayout.alignSelf, FlexboxLayout.AlignAuto) // Nothing but what it inherits from the parent
            compare(rectItem1.x, 0)
            compare(rectItem1.y, flexboxLayout.height / 2 - rectItem1.height / 2)
            compare(rectItem2.x, rectItem1.implicitWidth)
            compare(rectItem2.y, 0)

            rectItem1.destroy()
            rectItem2.destroy()
            flexboxLayout.destroy()
        }

        Component {
            id: flexboxLayoutComponent1
            FlexboxLayout {
                property int numGapChanged: 0
                property int numRowGapChanged: 0
                property int numColumnGapChanged: 0
                onGapChanged: { ++numGapChanged }
                onRowGapChanged: { ++numRowGapChanged }
                onColumnGapChanged: { ++numColumnGapChanged }
            }
        }

        function test_flexGap() {
            let flexboxLayout = createTemporaryObject(flexboxLayoutComponent1, container)
            flexboxLayout.width = 60
            flexboxLayout.height = 100
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.width, 60)
            compare(flexboxLayout.height, 100)

            // direction: FlexboxLayout.Row (default)
            // wrap: FlexboxLayout.NoWrap
            compare(flexboxLayout.direction, FlexboxLayout.Row)
            compare(flexboxLayout.wrap, FlexboxLayout.NoWrap)

            let rectItem1 = flexItem.createObject(flexboxLayout)
            waitForItemPolished(flexboxLayout)

            let rectItem2 = flexItem.createObject(flexboxLayout)
            waitForItemPolished(flexboxLayout)

            flexboxLayout.gap = 10
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.gap, 10)
            compare(flexboxLayout.numGapChanged, 1)
            compare(flexboxLayout.rowGap, 10)
            compare(flexboxLayout.numRowGapChanged, 1)
            compare(flexboxLayout.columnGap, 10)
            compare(flexboxLayout.numColumnGapChanged, 1)
            compare(rectItem1.x, 0)
            compare(rectItem1.y, 0)
            compare(rectItem2.x, rectItem1.implicitWidth + flexboxLayout.gap)
            compare(rectItem2.y, 0)

            // Override the gap property above with the columnGap
            flexboxLayout.columnGap = 20
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.gap, 10)
            compare(flexboxLayout.numGapChanged, 1)
            compare(flexboxLayout.rowGap, 10)
            compare(flexboxLayout.numRowGapChanged, 1)
            compare(flexboxLayout.columnGap, 20)
            compare(flexboxLayout.numColumnGapChanged, 2)
            compare(rectItem1.x, 0)
            compare(rectItem1.y, 0)
            compare(rectItem2.x, rectItem1.implicitWidth + flexboxLayout.columnGap)
            compare(rectItem2.y, 0)

            let rectItem3 = flexItem.createObject(flexboxLayout)
            rectItem3.Layout.preferredWidth = 30
            rectItem3.Layout.preferredHeight = 20

            // Wrap the items to consider the rowGap
            flexboxLayout.rowGap = 30
            flexboxLayout.wrap = FlexboxLayout.Wrap
            compare(flexboxLayout.wrap, FlexboxLayout.Wrap)
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.gap, 10)
            compare(flexboxLayout.numGapChanged, 1)
            compare(flexboxLayout.rowGap, 30)
            compare(flexboxLayout.numRowGapChanged, 2)
            compare(flexboxLayout.columnGap, 20)
            compare(flexboxLayout.numColumnGapChanged, 2)
            compare(rectItem1.x, 0)
            compare(rectItem1.y, 0)
            compare(rectItem2.x, rectItem1.implicitWidth + flexboxLayout.columnGap)
            compare(rectItem2.y, 0)
            compare(rectItem3.x, 0)
            compare(rectItem3.y, Math.max(rectItem1.height, rectItem2.height) + flexboxLayout.rowGap)

            flexboxLayout.columnGap = undefined
            compare(flexboxLayout.columnGap, 10)
            compare(flexboxLayout.numColumnGapChanged, 3)
            flexboxLayout.rowGap = undefined
            compare(flexboxLayout.rowGap, 10)
            compare(flexboxLayout.numRowGapChanged, 3)
            flexboxLayout.gap = undefined
            compare(flexboxLayout.gap, 0)
            compare(flexboxLayout.numGapChanged, 2)
            compare(flexboxLayout.rowGap, 0)
            compare(flexboxLayout.numRowGapChanged, 4)
            compare(flexboxLayout.columnGap, 0)
            compare(flexboxLayout.numColumnGapChanged, 4)

            rectItem1.destroy()
            rectItem2.destroy()
            rectItem3.destroy()
            flexboxLayout.destroy()
        }

        function test_itemVisibility() {
            let flexboxLayout = createTemporaryObject(dynamicFlexboxLayoutComponent, container)
            flexboxLayout.width = 60
            flexboxLayout.height = 100
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.width, 60)
            compare(flexboxLayout.height, 100)

            let rectItem1 = flexItem.createObject(flexboxLayout)
            waitForItemPolished(flexboxLayout)

            let rectItem2 = flexItem.createObject(flexboxLayout)
            waitForItemPolished(flexboxLayout)

            // direction: FlexboxLayout.Row (default)
            // wrap: FlexboxLayout.NoWrap
            compare(flexboxLayout.direction, FlexboxLayout.Row)
            compare(flexboxLayout.wrap, FlexboxLayout.NoWrap)

            rectItem1.visible = false
            waitForItemPolished(flexboxLayout)
            compare(rectItem1.visible, false)
            compare(rectItem2.visible, true)

            // rectItem2 is the only visible item within the layout
            compare(rectItem2.x, 0)
            compare(rectItem2.y, 0)

            rectItem1.visible = true
            waitForItemPolished(flexboxLayout)
            compare(rectItem1.visible, true)
            compare(rectItem2.visible, true)

            compare(rectItem1.x, 0)
            compare(rectItem1.y, 0)
            compare(rectItem2.x, rectItem1.width)
            compare(rectItem2.y, 0)
        }

        function test_respectLayoutAttachedProperties() {
            let flexboxLayout = createTemporaryObject(dynamicFlexboxLayoutComponent, container)
            flexboxLayout.width = 60
            flexboxLayout.height = 100
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.width, 60)
            compare(flexboxLayout.height, 100)

            let rectItem1 = flexItem.createObject(flexboxLayout)
            rectItem1.Layout.fillWidth = true
            rectItem1.Layout.maximumHeight = 100
            waitForItemPolished(flexboxLayout)

            let rectItem2 = flexItem.createObject(flexboxLayout)
            rectItem2.Layout.fillHeight = true
            rectItem2.Layout.minimumWidth = 20
            waitForItemPolished(flexboxLayout)

            // direction: FlexboxLayout.Row (default)
            // wrap: FlexboxLayout.NoWrap
            compare(flexboxLayout.direction, FlexboxLayout.Row)
            compare(flexboxLayout.wrap, FlexboxLayout.NoWrap)

            compare(rectItem1.width, 40)
            compare(rectItem1.height, 20)
            compare(rectItem2.width, 20)
            compare(rectItem2.height, 100)
        }

        function test_nestedFlexLayout() {
            let flexboxLayout = createTemporaryObject(dynamicFlexboxLayoutComponent, container)
            flexboxLayout.width = 200
            flexboxLayout.height = 200
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.width, 200)
            compare(flexboxLayout.height, 200)

            let nestedFlexboxLayout = dynamicFlexboxLayoutComponent.createObject(flexboxLayout)
            nestedFlexboxLayout.Layout.fillWidth = true
            nestedFlexboxLayout.Layout.fillHeight = true
            nestedFlexboxLayout.Layout.maximumHeight = 200
            nestedFlexboxLayout.Layout.maximumWidth = 100
            waitForItemPolished(flexboxLayout)
            compare(nestedFlexboxLayout.width, 100)
            compare(nestedFlexboxLayout.height, 200)

            // direction: FlexboxLayout.Row (default)
            // wrap: FlexboxLayout.NoWrap
            compare(nestedFlexboxLayout.direction, FlexboxLayout.Row)
            compare(nestedFlexboxLayout.wrap, FlexboxLayout.NoWrap)

            let rectItem1 = flexItem.createObject(nestedFlexboxLayout)
            rectItem1.Layout.fillWidth = true
            rectItem1.Layout.maximumWidth = 80
            rectItem1.Layout.minimumHeight = 100

            let rectItem2 = flexItem.createObject(nestedFlexboxLayout)
            rectItem2.Layout.fillHeight = true
            rectItem2.Layout.minimumWidth = 20
            rectItem2.Layout.maximumHeight = 100

            waitForItemPolished(flexboxLayout)

            compare(rectItem1.width, 80)
            compare(rectItem1.height, 100)
            compare(rectItem2.width, 20)
            compare(rectItem2.height, 100)
        }

        Component {
            id: flexboxWithSizeHintsLayoutComponent
            FlexboxLayout {
                gap: 0
                Rectangle {
                    Layout.minimumWidth: 1
                    Layout.minimumHeight: 1
                    implicitWidth: 10
                    implicitHeight: 10
                    Layout.maximumWidth: 100
                    Layout.maximumHeight: 100
                }
                Rectangle {
                    Layout.minimumWidth: 2
                    Layout.minimumHeight: 2
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                    Layout.maximumWidth: 200
                    Layout.maximumHeight: 200
                }
            }
        }

        function test_sizeHints() {
            let flexboxLayout = createTemporaryObject(flexboxWithSizeHintsLayoutComponent, container)
            waitForItemPolished(flexboxLayout)
            // Test size hints of main axis. Since nothing has Layout.fillWidth: true, no items will flex
            // It is therefore considered as a fixed-width layout
            expectFailContinue("", "Layout.minimumWidth is 2."
                + " FlexboxLayout should take fillWidth/sizePolicy into consideration when normalizing size hints")
            compare(flexboxLayout.Layout.minimumWidth, 30)
            compare(flexboxLayout.implicitWidth, 30)
            expectFailContinue("", "Layout.maximumWidth is ∞."
                + " FlexboxLayout should take fillWidth/sizePolicy into consideration when normalizing size hints")
            compare(flexboxLayout.Layout.maximumWidth, 30)

            // Make first item flexible
            let item0 = flexboxLayout.children[0]
            item0.Layout.fillWidth = true
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.Layout.minimumWidth, 3)
            compare(flexboxLayout.implicitWidth, 30)

            // The semantic of the implicit value of Layout.maximumWidth is unclear. Currently it is infinite.
            // But this is inconsistent with RowLayout:
            // RowLayout will have its maximumWidth be the sum of all the childrens *effective* maximum widths (+spacings)
            // (effective maximum width is just: (Layout.fillWidth ? Layout.maximumWidth : implicitWidth), but this is
            // beside the point here)

            // If the FlexboxLayout have the RowLayout semantic, and it is a child of another layout, the parent layout
            // will respect its  maximumWidth (so FlexboxLayout.justifyContent will be pointless)

            // However, if the FlexboxLayout is anchored to a parent Item with anchors.fill, the FlexboxLayout can become
            // wider than its maximumWidth. In that scenario, FlexboxLayout.justifyContent has a purpose
            compare(flexboxLayout.Layout.maximumWidth, 300)

            // restore back to no flexing
            item0.Layout.fillWidth = false
            waitForItemPolished(flexboxLayout)

            // Test size hints of cross-axis
            compare(flexboxLayout.Layout.minimumHeight, 2)
            compare(flexboxLayout.implicitHeight, 20)
             // actually semantic of this is unclear as described for Layout.maximumWidth above
            compare(flexboxLayout.Layout.maximumHeight, 200)

            // ----- gap: 5 -----
            // Now add a gap, test if size hints are adjusted accordingly
            flexboxLayout.gap = 5
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.Layout.minimumWidth, 8)
            compare(flexboxLayout.implicitWidth, 35)
            compare(flexboxLayout.Layout.maximumWidth, 305)

            // If the FlexboxLayout is not wrapping, gap shouldn't change the cross-axis size hint
            // Test size hints of cross-axis
            compare(flexboxLayout.Layout.minimumHeight, 2)
            compare(flexboxLayout.implicitHeight, 20)
             // actually semantic of this is unclear as described for Layout.maximumWidth above
            compare(flexboxLayout.Layout.maximumHeight, 200)


            // flex-wrap: wrap
            /*
            if (wrap)
                minimumWidth: width of the largest minimumwidth
                minimumHeight: height of the largest minimumHeight

                implicitWidth: width for being close to the golden ratio    (+ gaps)
                implicitWidth: height for being close to the golden ratio.  (+ gaps)

                maximumWidth: sum of all maximumWidths                      (+ gaps)
                maximumHeight: sum of all maximumHeights                    (+ gaps)
            */
            flexboxLayout.wrap = FlexboxLayout.Wrap
            waitForItemPolished(flexboxLayout)
            compare(flexboxLayout.Layout.minimumWidth, 2)
            compare(flexboxLayout.Layout.minimumHeight, 2)
            compare(flexboxLayout.implicitWidth, 30 + 5)
            compare(flexboxLayout.implicitHeight, 20)
            compare(flexboxLayout.Layout.maximumWidth, 300 + 5)
            compare(flexboxLayout.Layout.maximumHeight, 300 + 5)
        }
    }
}
