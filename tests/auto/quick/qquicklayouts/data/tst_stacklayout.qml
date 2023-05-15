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

        function geometry(item) {
            return [item.x, item.y, item.width, item.height]
        }

        Component {
            id: countGeometryChanges_Component
            StackLayout {
                id: stack
                property alias col: _col
                property alias row: _row
                width: 100
                ColumnLayout {
                    id: _col
                    property alias r1: _r1
                    property alias r2: _r2
                    property alias r3: _r3
                    spacing: 0
                    property int counter : 0
                    onWidthChanged: { ++counter; }
                    Rectangle {
                        id: _r1
                        implicitWidth: 20
                        implicitHeight: 20
                        Layout.fillWidth: true
                        property int counter : 0
                        onWidthChanged: { ++counter; }
                    }
                    Rectangle {
                        id: _r2
                        implicitWidth: 50
                        implicitHeight: 50
                        Layout.fillWidth: true
                        property int counter : 0
                        onWidthChanged: { ++counter; }
                    }
                    Rectangle {
                        id: _r3
                        implicitWidth: 40
                        implicitHeight: 40
                        Layout.fillWidth: true
                        property int counter : 0
                        onWidthChanged: { ++counter; }
                    }
                }
                RowLayout {
                    id: _row
                    property alias r5: _r5
                    spacing: 0
                    property int counter : 0
                    onWidthChanged: { ++counter; }
                    Rectangle {
                        id: _r5
                        implicitWidth: 100
                        implicitHeight: 100
                        Layout.fillWidth: true
                        property int counter : 0
                        onWidthChanged: { ++counter; }
                    }
                }
            }
        }

        function test_countGeometryChanges() {

            var stack = countGeometryChanges_Component.createObject(container)
            compare(stack.currentIndex, 0)
            compare(stack.col.width, 100)
            compare(stack.col.height, 110)
            compare(stack.row.width, 100)
            compare(stack.row.height, 100)
            verify(stack.col.r1.counter <= 2)
            compare(stack.col.r2.counter, 1)
            verify(stack.col.r3.counter <= 2)
            verify(stack.col.counter <= 2)
            compare(stack.row.counter, 1)    // not visible, will only receive the initial geometry change
            compare(stack.row.r5.counter, 0)
            stack.destroy()
        }


        Component {
            id: layoutItem_Component
            Rectangle {
                implicitWidth: 20
                implicitHeight: 20
            }
        }

        Component {
            id: emtpy_StackLayout_Component
            StackLayout {
                property int num_onCountChanged: 0
                property int num_onCurrentIndexChanged: 0
                onCountChanged: { ++num_onCountChanged; }
                onCurrentIndexChanged: { ++num_onCurrentIndexChanged; }
            }
        }

        function test_addAndRemoveItems()
        {
            var stack = emtpy_StackLayout_Component.createObject(container)
            stack.currentIndex = 2
            compare(stack.implicitWidth, 0)
            compare(stack.implicitHeight, 0)

            var rect0 = layoutItem_Component.createObject(stack)
            waitForPolish(stack)
            compare(stack.implicitWidth, 20)
            compare(stack.implicitHeight, 20)
            compare(rect0.visible, false)

            var rect1 = layoutItem_Component.createObject(stack)
            rect1.Layout.preferredWidth = 30
            rect1.Layout.preferredHeight = 10
            waitForPolish(stack)
            compare(stack.implicitWidth, 30)
            compare(stack.implicitHeight, 20)
            compare(rect0.visible, false)
            compare(rect1.visible, false)

            var rect2 = layoutItem_Component.createObject(stack)
            rect2.x = 42    // ### items in a stacklayout will have their x and y positions discarded.
            rect2.y = 42
            rect2.Layout.preferredWidth = 80
            rect2.Layout.preferredHeight = 30
            rect2.Layout.fillWidth = true
            waitForPolish(stack)
            compare(stack.implicitWidth, 80)
            compare(stack.implicitHeight, 30)
            compare(rect0.visible, false)
            compare(rect1.visible, false)
            compare(rect2.visible, true)
            compare(geometry(rect2), geometry(stack))

            rect2.destroy()
            wait(0)
            waitForPolish(stack)
            compare(stack.implicitWidth, 30)
            compare(stack.implicitHeight, 20)

            rect0.destroy()
            wait(0)
            waitForPolish(stack)
            compare(stack.implicitWidth, 30)
            compare(stack.implicitHeight, 10)

            rect1.destroy()
            wait(0)
            waitForPolish(stack)
            compare(stack.implicitWidth, 0)
            compare(stack.implicitHeight, 0)

            stack.destroy()
        }

        function test_sizeHint_data() {
            return [
                    { tag: "propagateNone",            layoutHints: [10, 20, 30], childHints: [11, 21, 31], expected:[10, 20, Number.POSITIVE_INFINITY]},
                    { tag: "propagateMinimumWidth",    layoutHints: [-1, 20, 30], childHints: [10, 21, 31], expected:[10, 20, Number.POSITIVE_INFINITY]},
                    { tag: "propagatePreferredWidth",  layoutHints: [10, -1, 30], childHints: [11, 20, 31], expected:[10, 20, Number.POSITIVE_INFINITY]},
                    { tag: "propagateMaximumWidth",    layoutHints: [10, 20, -1], childHints: [11, 21, 30], expected:[10, 20, Number.POSITIVE_INFINITY]},
                    { tag: "propagateAll",             layoutHints: [-1, -1, -1], childHints: [10, 20, 30], expected:[10, 20, Number.POSITIVE_INFINITY]},
                    { tag: "propagateCrazy",           layoutHints: [-1, -1, -1], childHints: [40, 21, 30], expected:[30, 30, Number.POSITIVE_INFINITY]},
                    { tag: "expandMinToExplicitPref",  layoutHints: [-1,  1, -1], childHints: [11, 21, 31], expected:[ 1,  1, Number.POSITIVE_INFINITY]},
                    { tag: "expandMaxToExplicitPref",  layoutHints: [-1, 99, -1], childHints: [11, 21, 31], expected:[11, 99, Number.POSITIVE_INFINITY]},
                    { tag: "expandAllToExplicitMin",   layoutHints: [99, -1, -1], childHints: [11, 21, 31], expected:[99, 99, Number.POSITIVE_INFINITY]},
                    { tag: "expandPrefToExplicitMin",  layoutHints: [24, -1, -1], childHints: [11, 21, 31], expected:[24, 24, Number.POSITIVE_INFINITY]},
                    { tag: "boundPrefToExplicitMax",   layoutHints: [-1, -1, 19], childHints: [11, 21, 31], expected:[11, 19, Number.POSITIVE_INFINITY]},
                    { tag: "boundAllToExplicitMax",    layoutHints: [-1, -1,  9], childHints: [11, 21, 31], expected:[ 9,  9, Number.POSITIVE_INFINITY]},
                    ];
        }

        function itemSizeHints(item) {
            return [item.Layout.minimumWidth, item.implicitWidth, item.Layout.maximumWidth]
        }
        Component {
            id: stacklayout_sizeHint_Component
            StackLayout {
                property int implicitWidthChangedCount : 0
                onImplicitWidthChanged: { ++implicitWidthChangedCount }
                ColumnLayout {
                    Rectangle {
                        id: r1
                        color: "red"
                        Layout.minimumWidth: 1
                        Layout.preferredWidth: 2
                        Layout.maximumWidth: 3

                        Layout.minimumHeight: 20
                        Layout.preferredHeight: 20
                        Layout.maximumHeight: 20
                        Layout.fillWidth: true
                    }
                }
            }
        }

        function test_sizeHint(data) {
            var layout = stacklayout_sizeHint_Component.createObject(container)

            var col = layout.children[0]
            col.Layout.minimumWidth = data.layoutHints[0]
            col.Layout.preferredWidth = data.layoutHints[1]
            col.Layout.maximumWidth = data.layoutHints[2]

            var child = col.children[0]
            if (data.implicitWidth !== undefined) {
                child.implicitWidth = data.implicitWidth
            }
            child.Layout.minimumWidth = data.childHints[0]
            child.Layout.preferredWidth = data.childHints[1]
            child.Layout.maximumWidth = data.childHints[2]

            waitForPolish(layout)
            var effectiveSizeHintResult = [layout.Layout.minimumWidth, layout.implicitWidth, layout.Layout.maximumWidth]
            compare(effectiveSizeHintResult, data.expected)
            layout.destroy()
        }

        Component {
            id: stacklayout_addIgnoredItem_Component
            StackLayout {
                Repeater {
                    id: rep
                    model: 1
                    Rectangle {
                        id: r
                    }
                }
            }
        }

        // Items with no size information is ignored.
        function test_addIgnoredItem()
        {
            var stack = stacklayout_addIgnoredItem_Component.createObject(container)
            compare(stack.count, 1)
            compare(stack.implicitWidth, 0)
            compare(stack.implicitHeight, 0)
            var r = stack.children[0]
            r.Layout.preferredWidth = 20
            r.Layout.preferredHeight = 30
            waitForPolish(stack)
            compare(stack.count, 1)
            compare(stack.implicitWidth, 20)
            compare(stack.implicitHeight, 30)
            stack.destroy();
        }

        function test_dontCrashWhenAnchoredToAWindow() {
            var test_layoutStr =
           'import QtQuick;                             \
            import QtQuick.Window;                      \
            import QtQuick.Layouts;                     \
            Window {                                    \
                visible: true;                          \
                width: stack.implicitWidth;             \
                height: stack.implicitHeight;           \
                StackLayout {                           \
                    id: stack;                          \
                    currentIndex: 0;                    \
                    anchors.fill: parent;               \
                    Rectangle {                         \
                        color: "red";                   \
                        implicitWidth: 300;             \
                        implicitHeight: 200;            \
                    }                                   \
                }                                       \
            }                                           '

            var win = Qt.createQmlObject(test_layoutStr, container, '');
            if (win.visibility === Window.Windowed) {
                // on single-window systems (such as Android), the window geometry will be
                // fullscreen, and most likely it will be set to screen size. Avoid this test for
                // those systems, as the size of the window will not be determined by the layout
                tryCompare(win, 'width', 300);
            }
            win.destroy()
        }

        Component {
            id: test_dontCrashWhenChildIsResizedToNull_Component
            StackLayout {
                property alias rect : _rect
                Rectangle {
                    id: _rect;
                    color: "red"
                    implicitWidth: 200
                    implicitHeight: 200
                }
            }
        }

        function test_dontCrashWhenChildIsResizedToNull() {
            var layout = test_dontCrashWhenChildIsResizedToNull_Component.createObject(container)
            layout.rect.width = 0
            layout.width = 222      // trigger a rearrange with a valid size
            layout.height = 222
        }

        Component {
            id: test_currentIndex_Component
            StackLayout {
                currentIndex: 1
                Text {
                    text: "0"
                }
                Text {
                    text: "1"
                }
            }
        }

        function test_currentIndex() {
            var layout = test_currentIndex_Component.createObject(container)
            var c0 = layout.children[0]
            var c1 = layout.children[1]
            compare(layout.currentIndex, 1)
            tryCompare(layout, 'visible', true)
            compare(c0.visible, false)
            compare(c1.visible, true)
            layout.currentIndex = 0
            compare(c0.visible, true)
            compare(c1.visible, false)
            var c2 = layoutItem_Component.createObject(layout)
            compare(c2.visible, false)

            /*
             * destroy the current item and check if visibility advances to next
             */
            c0.destroy()
            tryCompare(c1, 'visible', true)
            compare(c2.visible, false)
            c1.destroy()
            tryCompare(c2, 'visible', true)
            c2.destroy()
            tryCompare(layout, 'currentIndex', 0)

            layout.destroy()

            /*
             * Test the default/implicit value of currentIndex, either -1 (if empty) or 0:
             */
            layout = emtpy_StackLayout_Component.createObject(container)
            tryCompare(layout, 'visible', true)
            compare(layout.currentIndex, -1)
            compare(layout.num_onCurrentIndexChanged, 0)
            // make it non-empty
            c0 = layoutItem_Component.createObject(layout)
            compare(layout.currentIndex, 0)
            compare(layout.num_onCurrentIndexChanged, 1)
            compare(c0.visible, true)
            // make it empty again
            c0.destroy()
            wait(0)
            compare(layout.currentIndex, -1)
            //tryCompare(layout, 'currentIndex', -1)
            compare(layout.num_onCurrentIndexChanged, 2)

            /*
             * Check that explicit value doesn't change,
             * and that no items are visible if the index is invalid/out of range
             */
            layout.currentIndex = 2
            compare(layout.currentIndex, 2)
            compare(layout.num_onCurrentIndexChanged, 3)
            c0 = layoutItem_Component.createObject(layout)
            compare(layout.currentIndex, 2)
            compare(c0.visible, false)

            c1 = layoutItem_Component.createObject(layout)
            compare(layout.currentIndex, 2)
            compare(c0.visible, false)
            compare(c1.visible, false)

            c2 = layoutItem_Component.createObject(layout)
            compare(layout.currentIndex, 2)
            compare(c0.visible, false)
            compare(c1.visible, false)
            compare(c2.visible, true)

            c2.destroy()
            wait(0)
            compare(layout.currentIndex, 2)
            compare(c0.visible, false)
            compare(c1.visible, false)
            c1.destroy()
            wait(0)
            compare(layout.currentIndex, 2)
            compare(c0.visible, false)
            c0.destroy()
            wait(0)
            compare(layout.currentIndex, 2)
            compare(layout.num_onCurrentIndexChanged, 3)
        }

        function test_count() {
            var layout = emtpy_StackLayout_Component.createObject(container)
            tryCompare(layout, 'visible', true)
            compare(layout.count, 0)
            compare(layout.currentIndex, -1)
            compare(layout.num_onCountChanged, 0)
            compare(layout.num_onCurrentIndexChanged, 0)
            var c0 = layoutItem_Component.createObject(layout)
            compare(layout.count, 1)
            compare(layout.currentIndex, 0)
            compare(layout.num_onCurrentIndexChanged, 1)
            compare(layout.num_onCountChanged, 1)
        }

        // QTBUG-111902
        Component {
            id: stackComponent
            Loader {
                id: loader
                asynchronous: true
                sourceComponent: StackLayout {
                    id: stackLayout
                    Repeater {
                        model: 3
                        Item {
                            required property int index
                        }
                    }
                }
            }
        }

        function test_loadStackLayoutAsynchronously() {
            var loaderObj = stackComponent.createObject(container)
            // Check for loader status to be ready
            tryCompare(loaderObj, 'status', 1)
            // Get stack layout object
            var stackLayoutObj = loaderObj.item
            // Check repeater index of child object
            compare(stackLayoutObj.children[0].index, 0)
            compare(stackLayoutObj.children[1].index, 1)
            compare(stackLayoutObj.children[2].index, 2)
            // Check stack layout attached property index
            compare(stackLayoutObj.children[0].StackLayout.index, 0)
            compare(stackLayoutObj.children[1].StackLayout.index, 1)
            compare(stackLayoutObj.children[2].StackLayout.index, 2)
        }

        Component {
            id: test_repeater_Component

            Item {
                property alias stackLayout : stackLayout
                property var model : ListModel {
                    /*
                     * We cannot programmatically reorder siblings (QQuickItem::stackBefore()
                     * and QQuickItem::stackAfter() are not not available to QML, and we cannot
                     * alter the Item::children property to reorder siblings)
                     * Therefore, we have to go through the hoops with a ListModel and Repeater in
                     * order to trigger sibling reordering, just as reported in QTBUG-112691.
                     * Adding an item to a specific index (with model.insert()), will be done in
                     * two steps:
                     *  1. Append an Item to be the last of the siblings
                     *  2. Reorder that Rectangle to be at the correct child index that corresponds
                     *     to the index given to model.insert()
                     *
                     * Adding an item to a specific index will therefore test sibling reordering
                     */
                    id: listModel
                }
                StackLayout {
                    id: stackLayout
                    anchors.fill: parent
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Repeater {
                        id: repeater
                        model:listModel
                        delegate: Rectangle {
                            implicitWidth: 100
                            implicitHeight: 100
                            objectName: model.color
                            color: model.color
                        }
                    }
                }
            }
        }

        function test_repeater() {
            let item = createTemporaryObject(test_repeater_Component, container)
            let layout = item.stackLayout
            let model = item.model
            function verifyVisibilityOfItems() {
                for (let i = 0; i < layout.count; ++i) {
                    compare(layout.children[i].visible, layout.currentIndex === i)
                }
            }

            compare(layout.currentIndex, -1)
            compare(layout.count, 0)

            model.append({ "color": "red" })
            compare(layout.currentIndex, 0)
            compare(layout.count, 1)
            verifyVisibilityOfItems()

            model.append({ "color": "green" })
            compare(layout.currentIndex, 0)
            compare(layout.count, 2)
            verifyVisibilityOfItems()

            model.append({ "color": "blue" })
            compare(layout.currentIndex, 0)
            compare(layout.count, 3)
            verifyVisibilityOfItems()

            model.insert(0, { "color": "black" })
            compare(layout.currentIndex, 1)
            compare(layout.count, 4)
            verifyVisibilityOfItems()

            // An implicit currentIndex will reset back to -1 if
            // the StackLayout is empty
            model.clear()
            compare(layout.currentIndex, -1)
            compare(layout.count, 0)

            // set explicit index to out of bounds
            layout.currentIndex = 1
            compare(layout.currentIndex, 1)
            compare(layout.count, 0)
            verifyVisibilityOfItems()

            model.append({ "color": "red" })
            compare(layout.currentIndex, 1)
            compare(layout.count, 1)
            verifyVisibilityOfItems()

            model.append({ "color": "green" })
            compare(layout.currentIndex, 1)
            compare(layout.count, 2)
            verifyVisibilityOfItems()

            model.insert(1, { "color": "brown" })
            compare(layout.currentIndex, 2)
            compare(layout.count, 3)
            verifyVisibilityOfItems()

            // remove red, currentIndex should decrease
            model.remove(0, 1)
            compare(layout.currentIndex, 1)
            compare(layout.count, 2)
            verifyVisibilityOfItems()
        }
    }
}
