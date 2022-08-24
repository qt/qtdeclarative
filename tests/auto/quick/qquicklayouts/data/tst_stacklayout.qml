// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.15
import QtTest 1.15
import QtQuick.Layouts 1.15

Item {
    id: container
    width: 200
    height: 200
    TestCase {
        id: testCase
        name: "Tests_StackLayout"
        when: windowShown
        width: 200
        height: 200

        Component {
            id: layout_rearrange_Component

            StackLayout {
                width: 640
                height: 480

                property alias testRectangle: testRectangle

                RowLayout {
                    spacing: 0

                    Rectangle {
                        Layout.preferredWidth: 100
                        Layout.preferredHeight: 100
                    }

                    Rectangle {
                        id: testRectangle
                        Layout.preferredWidth: 100
                        Layout.preferredHeight: 100
                        visible: false
                    }

                    Item {
                        Layout.fillWidth: true
                    }
                }
            }
        }

        Component {
            id: stackLayoutComponent

            StackLayout {}
        }

        Component {
            id: itemComponent

            Item {}
        }

        Component {
            id: signalSpyComponent

            SignalSpy {}
        }

        function test_rearrange()
        {
            var layout = layout_rearrange_Component.createObject(container)
            compare(layout.testRectangle.x, 0)
            layout.testRectangle.visible = true
            tryCompare(layout.testRectangle, "x", 100)

            layout.destroy()
        }

        // Test that items added dynamically to the StackLayout
        // have valid attached properties when accessed imperatively.
        function test_attachedDynamicImperative() {
            let layout = createTemporaryObject(stackLayoutComponent, container, { "anchors.fill": parent })
            verify(layout)

            let item1 = itemComponent.createObject(layout, { objectName: "item1" })
            verify(item1)
            compare(item1.StackLayout.index, 0)
            compare(item1.StackLayout.isCurrentItem, true)
            compare(item1.StackLayout.layout, layout)

            let item2 = itemComponent.createObject(layout, { objectName: "item2" })
            verify(item2)
            compare(item2.StackLayout.index, 1)
            compare(item2.StackLayout.isCurrentItem, false)
            compare(item2.StackLayout.layout, layout)

            // Test creating an item without a parent, accessing the attached properties,
            // and _then_ add it to the StackLayout and check its attached properties again.
            let item3 = itemComponent.createObject(null, { objectName: "item3" })
            verify(item3)
            compare(item3.StackLayout.index, -1)
            compare(item3.StackLayout.isCurrentItem, false)
            compare(item3.StackLayout.layout, null)

            let signalSpy = signalSpyComponent.createObject(item3,
                { target: item3.StackLayout, signalName: "indexChanged" })
            verify(signalSpy)
            verify(signalSpy.valid)

            item3.parent = layout
            compare(item3.StackLayout.index, 2)
            compare(item3.StackLayout.isCurrentItem, false)
            compare(item3.StackLayout.layout, layout)
            compare(signalSpy.count, 1)
        }

        Component {
            id: attachedPropertiesItemComponent

            Item {
                readonly property int index: StackLayout.index
                readonly property bool isCurrentItem: StackLayout.isCurrentItem
                readonly property StackLayout layout: StackLayout.layout
            }
        }

        // Test that items added dynamically to the StackLayout
        // have valid attached properties when accessed declaratively.
        function test_attachedDynamicDeclarative() {
            let layout = createTemporaryObject(stackLayoutComponent, container, { "anchors.fill": parent })
            verify(layout)

            let item1 = attachedPropertiesItemComponent.createObject(layout, { objectName: "item1" })
            verify(item1)
            compare(item1.index, 0)
            compare(item1.isCurrentItem, true)
            compare(item1.layout, layout)

            let item2 = attachedPropertiesItemComponent.createObject(layout, { objectName: "item2" })
            verify(item2)
            compare(item2.index, 1)
            compare(item2.isCurrentItem, false)
            compare(item2.layout, layout)
        }

        Component {
            id: tabComponent
            Rectangle {
                color: "#ff0000"
            }
        }

        function test_attachedDynamicRendered() {
            let layout = createTemporaryObject(stackLayoutComponent, container, { "anchors.fill": parent })
            verify(layout)

            let item1 = tabComponent.createObject(layout, { objectName: "item1" })
            verify(item1)
            compare(item1.StackLayout.index, 0)
            compare(item1.StackLayout.isCurrentItem, true)
            compare(item1.StackLayout.layout, layout)

            tryCompare(item1, "width", 200)
            tryCompare(item1, "height", 200)
        }

        Component {
            id: attachedStackLayoutComponent

            StackLayout {
                anchors.fill: parent

                property alias item1: item1
                property alias item2: item2

                Item {
                    id: item1
                    readonly property int index: StackLayout.index
                    readonly property bool isCurrentItem: StackLayout.isCurrentItem
                    readonly property StackLayout layout: StackLayout.layout
                }
                Item {
                    id: item2
                    readonly property int index: StackLayout.index
                    readonly property bool isCurrentItem: StackLayout.isCurrentItem
                    readonly property StackLayout layout: StackLayout.layout
                }
            }
        }

        // Test that items that are declared statically within StackLayout
        // have valid attached properties when accessed declaratively.
        function test_attachedStaticDeclarative() {
            let layout = createTemporaryObject(attachedStackLayoutComponent, container)
            verify(layout)
            compare(layout.item1.index, 0)
            compare(layout.item1.isCurrentItem, true)
            compare(layout.item1.layout, layout)
            compare(layout.item2.index, 1)
            compare(layout.item2.isCurrentItem, false)
            compare(layout.item2.layout, layout)
        }

        // Tests attached properties after adding and removing items.
        function test_attachedAddAndRemove() {
            let layout = createTemporaryObject(attachedStackLayoutComponent, container)
            verify(layout)
            compare(layout.item1.index, 0)
            compare(layout.item1.isCurrentItem, true)
            compare(layout.item1.layout, layout)
            compare(layout.item2.index, 1)
            compare(layout.item2.isCurrentItem, false)
            compare(layout.item2.layout, layout)

            // Remove item1. It's index should become -1.
            layout.item1.parent = null
            compare(layout.item1.index, -1)
            compare(layout.item1.isCurrentItem, false)
            compare(layout.item1.layout, null)
            compare(layout.item2.index, 0)
            compare(layout.item2.isCurrentItem, true)
            compare(layout.item2.layout, layout)

            // Add it back. Since it's appended, and item2 took index 0, its index should now be 1.
            layout.item1.parent = layout
            compare(layout.item1.index, 1)
            compare(layout.item1.isCurrentItem, false)
            compare(layout.item1.layout, layout)
            compare(layout.item2.index, 0)
            compare(layout.item2.isCurrentItem, true)
            compare(layout.item2.layout, layout)

            // Do the same with item2.
            layout.item2.parent = null
            compare(layout.item1.index, 0)
            compare(layout.item1.isCurrentItem, true)
            compare(layout.item1.layout, layout)
            compare(layout.item2.index, -1)
            compare(layout.item2.isCurrentItem, false)
            compare(layout.item2.layout, null)

            layout.item2.parent = layout
            compare(layout.item1.index, 0)
            compare(layout.item1.isCurrentItem, true)
            compare(layout.item1.layout, layout)
            compare(layout.item2.index, 1)
            compare(layout.item2.isCurrentItem, false)
            compare(layout.item2.layout, layout)
        }

        function test_implicitSize() {
            let layout = createTemporaryObject(stackLayoutComponent, container)
            verify(layout)
            let item1 = itemComponent.createObject(layout, { objectName: "item1", implicitWidth: 10, implicitHeight: 10 })
            verify(item1)
            compare(item1.implicitWidth, 10)
            compare(item1.implicitHeight, 10)
            let item2 = itemComponent.createObject(layout, { objectName: "item2", implicitWidth: 20, implicitHeight: 20 })
            verify(item2)
            compare(item2.implicitWidth, 20)
            compare(item2.implicitHeight, 20)
            verify(isPolishScheduled(layout))
            verify(waitForItemPolished(layout))
            compare(layout.implicitWidth, 20)
            compare(layout.implicitHeight, 20)
        }

        Component {
            id: layout_setCurrentIndex_Component

            StackLayout {
                width: 200
                height: 200

                property alias firstItem : rect
                property alias secondItem: rowLayout

                Rectangle {
                    id: rect
                    color: "red"
                    implicitWidth: 10
                    implicitHeight: 10
                }
                RowLayout {
                    id: rowLayout
                    spacing: 0
                    Rectangle {
                        color: "green"
                        implicitWidth: 10
                        implicitHeight: 10
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }
                    Rectangle {
                        color: "blue"
                        implicitWidth: 10
                        implicitHeight: 10
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }
                }
            }
        }

        function test_setCurrentIndex()
        {
            var layout = layout_setCurrentIndex_Component.createObject(container)
            compare(layout.firstItem.width, 200)

            // Invalidate the StackLayout (and its cached size hints)
            layout.firstItem.implicitWidth = 42

            layout.currentIndex = 1
            compare(layout.secondItem.width, 200)   // width should not be -1
            layout.destroy()
        }

    }
}
