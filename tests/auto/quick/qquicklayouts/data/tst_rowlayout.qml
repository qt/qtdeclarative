// Copyright (C) 2020 The Qt Company Ltd.
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
        name: "Tests_RowLayout"
        when: windowShown
        width: 200
        height: 200

        function itemRect(item)
        {
            return [item.x, item.y, item.width, item.height];
        }

        function cleanup() {
            if (LayoutSetup.useDefaultSizePolicy) {
                LayoutSetup.useDefaultSizePolicy = false
                compare(LayoutSetup.useDefaultSizePolicy, false)
            }
        }

        Component {
            id: rectangle_Component
            Rectangle {
                width: 100
                height: 50
            }
        }

        Component {
            id: layout_rowLayout_Component
            RowLayout {
            }
        }

        Component {
            id: layout_columnLayout_Component
            ColumnLayout {
            }
        }

        Component {
            id: itemsWithAnchorsLayout_Component
            RowLayout {
                spacing: 2
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
                                 + "This is undefined behavior; use Layout.alignment instead.")
            for (var i = 0; i < 7; ++i) {
                ignoreWarning(regex)
            }
            var layout = itemsWithAnchorsLayout_Component.createObject(container)
            waitForRendering(layout)
            layout.destroy()
        }

        function test_fixedAndExpanding() {
            var test_layoutStr =
               'import QtQuick 2.2;                     \
                import QtQuick.Layouts 1.0;             \
                RowLayout {                             \
                    id: row;                            \
                    width: 15;                          \
                    spacing: 0;                         \
                    property alias r1: _r1;             \
                    Rectangle {                         \
                        id: _r1;                        \
                        width: 5;                       \
                        height: 10;                     \
                        color: "#8080ff";               \
                        Layout.fillWidth: false         \
                    }                                   \
                    property alias r2: _r2;             \
                    Rectangle {                         \
                        id: _r2;                        \
                        width: 10;                      \
                        height: 20;                     \
                        color: "#c0c0ff";               \
                        Layout.fillWidth: true          \
                    }                                   \
                }                                       '

            var lay = Qt.createQmlObject(test_layoutStr, container, '');
            tryCompare(lay, 'implicitWidth', 15);
            compare(lay.implicitHeight, 20);
            compare(lay.height, 20);
            lay.width = 30
            compare(lay.r1.x, 0);
            compare(lay.r1.width, 5);
            compare(lay.r2.x, 5);
            compare(lay.r2.width, 25);
            lay.destroy()
        }

        function test_allExpanding() {
            var test_layoutStr =
               'import QtQuick 2.2;                     \
                import QtQuick.Layouts 1.0;             \
                RowLayout {                             \
                    id: row;                            \
                    width: 15;                          \
                    spacing: 0;                         \
                    property alias r1: _r1;             \
                    Rectangle {                         \
                        id: _r1;                        \
                        width: 5;                       \
                        height: 10;                     \
                        color: "#8080ff";               \
                        Layout.fillWidth: true          \
                    }                                   \
                    property alias r2: _r2;             \
                    Rectangle {                         \
                        id: _r2;                        \
                        width: 10;                      \
                        height: 20;                     \
                        color: "#c0c0ff";               \
                        Layout.fillWidth: true          \
                    }                                   \
                }                                       '

            var tmp = Qt.createQmlObject(test_layoutStr, container, '');
            waitForRendering(tmp)
            compare(tmp.implicitWidth, 15);
            compare(tmp.height, 20);
            tmp.width = 30
            compare(tmp.r1.width, 10);
            compare(tmp.r2.width, 20);
            compare(tmp.Layout.minimumWidth, 0)
            compare(tmp.Layout.maximumWidth, Number.POSITIVE_INFINITY)
            tmp.destroy()
        }

        function test_initialNestedLayouts() {
            var test_layoutStr =
               'import QtQuick 2.2;                             \
                import QtQuick.Layouts 1.0;                     \
                ColumnLayout {                                  \
                    id : col;                                   \
                    property alias row: _row;                   \
                    objectName: "col";                          \
                    anchors.fill: parent;                       \
                    RowLayout {                                 \
                        id : _row;                              \
                        property alias r1: _r1;                 \
                        property alias r2: _r2;                 \
                        objectName: "row";                      \
                        spacing: 0;                             \
                        Rectangle {                             \
                            id: _r1;                            \
                            color: "red";                       \
                            implicitWidth: 50;                  \
                            implicitHeight: 20;                 \
                        }                                       \
                        Rectangle {                             \
                            id: _r2;                            \
                            color: "green";                     \
                            implicitWidth: 50;                  \
                            implicitHeight: 20;                 \
                            Layout.fillWidth: true;             \
                        }                                       \
                    }                                           \
                }                                               '
            var col = Qt.createQmlObject(test_layoutStr, container, '');
            tryCompare(col, 'width', 200);
            tryCompare(col.row, 'width', 200);
            tryCompare(col.row.r1, 'width', 50);
            tryCompare(col.row.r2, 'width', 150);
            col.destroy()
        }

        Component {
            id: propagateImplicitWidthToParent_Component
            Item {
                width: 200
                height: 20

                // These might trigger a updateLayoutItems() before its component is completed...
                implicitWidth: row.implicitWidth
                implicitHeight: row.implicitHeight
                RowLayout {
                    id : row
                    anchors.fill: parent
                    property alias r1: _r1
                    property alias r2: _r2
                    spacing: 0
                    Rectangle {
                        id: _r1
                        color: "red"
                        implicitWidth: 50
                        implicitHeight: 20
                    }
                    Rectangle {
                        id: _r2
                        color: "green"
                        implicitWidth: 50
                        implicitHeight: 20
                        Layout.fillWidth: true
                    }
                }
            }
        }

        function test_propagateImplicitWidthToParent() {
            var item = createTemporaryObject(propagateImplicitWidthToParent_Component, container)
            var row = item.children[0]
            compare(row.width, 200)
            compare(itemRect(row.r1), [0, 0, 50, 20])
            compare(itemRect(row.r2), [50, 0, 150, 20])
        }

        function test_implicitSize() {
            var test_layoutStr =
               'import QtQuick 2.2;                             \
                import QtQuick.Layouts 1.0;                     \
                RowLayout {                                     \
                    id: row;                                    \
                    objectName: "row";                          \
                    spacing: 0;                                 \
                    height: 30;                                 \
                    anchors.left: parent.left;                  \
                    anchors.right: parent.right;                \
                    Rectangle {                                 \
                        color: "red";                           \
                        height: 2;                              \
                        Layout.minimumWidth: 50;                \
                    }                                           \
                    Rectangle {                                 \
                        color: "green";                         \
                        width: 10;                              \
                        Layout.minimumHeight: 4;                \
                    }                                           \
                    Rectangle {                                 \
                        implicitWidth: 1000;                    \
                        Layout.maximumWidth: 40;                \
                        implicitHeight: 6                       \
                    }                                           \
                }                                               '
            var row = Qt.createQmlObject(test_layoutStr, container, '');
            compare(row.implicitWidth, 50 + 10 + 40);
            compare(row.implicitHeight, 6);
            var r2 = row.children[2]
            r2.implicitWidth = 20
            waitForItemPolished(row)
            compare(row.implicitWidth, 50 + 10 + 20)
            var r3 = rectangle_Component.createObject(container)
            r3.implicitWidth = 30
            r3.parent = row
            waitForItemPolished(row)
            compare(row.implicitWidth, 50 + 10 + 20 + 30)
            row.destroy()
        }

        function test_countGeometryChanges() {
            var test_layoutStr =
               'import QtQuick 2.2;                             \
                import QtQuick.Layouts 1.0;                     \
                ColumnLayout {                                  \
                    id : col;                                   \
                    property alias row: _row;                   \
                    objectName: "col";                          \
                    anchors.fill: parent;                       \
                    RowLayout {                                 \
                        id : _row;                              \
                        property alias r1: _r1;                 \
                        property alias r2: _r2;                 \
                        objectName: "row";                      \
                        spacing: 0;                             \
                        property int counter : 0;               \
                        onWidthChanged: { ++counter; }          \
                        Rectangle {                             \
                            id: _r1;                            \
                            color: "red";                       \
                            implicitWidth: 50;                  \
                            implicitHeight: 20;                 \
                            property int counter : 0;           \
                            onWidthChanged: { ++counter; }      \
                            Layout.fillWidth: true;             \
                        }                                       \
                        Rectangle {                             \
                            id: _r2;                            \
                            color: "green";                     \
                            implicitWidth: 50;                  \
                            implicitHeight: 20;                 \
                            property int counter : 0;           \
                            onWidthChanged: { ++counter; }      \
                            Layout.fillWidth: true;             \
                        }                                       \
                    }                                           \
                }                                               '
            var col = Qt.createQmlObject(test_layoutStr, container, '');
            compare(col.width, 200);
            compare(col.row.width, 200);
            compare(col.row.r1.width, 100);
            compare(col.row.r2.width, 100);
            compare(col.row.r1.counter, 1);
            compare(col.row.r2.counter, 1);
            verify(col.row.counter <= 2);
            col.destroy()
        }

        function test_dynamicSizeAdaptationsForInitiallyInvisibleItemsInLayout() {
            var test_layoutStr =
               'import QtQuick 2.2;                     \
                import QtQuick.Layouts 1.0;             \
                RowLayout {                             \
                    id: row;                            \
                    width: 10;                          \
                    spacing: 0;                         \
                    property alias r1: _r1;             \
                    Rectangle {                         \
                        id: _r1;                        \
                        visible: false;                 \
                        height: 10;                     \
                        Layout.fillWidth: true;         \
                        color: "#8080ff";               \
                    }                                   \
                    property alias r2: _r2;             \
                    Rectangle {                         \
                        id: _r2;                        \
                        height: 10;                     \
                        Layout.fillWidth: true;         \
                        color: "#c0c0ff";               \
                    }                                   \
                }                                       '

            var lay = Qt.createQmlObject(test_layoutStr, container, '');
            compare(lay.r1.width, 0)
            compare(lay.r2.width, 10)
            lay.r1.visible = true;
            waitForRendering(lay)
            compare(lay.r1.width, 5)
            compare(lay.r2.width, 5)
            lay.destroy()
        }

        Component {
            id: layoutItem_Component
            Rectangle {
                implicitWidth: 20
                implicitHeight: 20
            }
        }

        Component {
            id: columnLayoutItem_Component
            ColumnLayout {
                spacing: 0
            }
        }

        Component {
            id: layout_addAndRemoveItems_Component
            RowLayout {
                spacing: 0
            }
        }

        function test_addAndRemoveItems()
        {
            var layout = createTemporaryObject(layout_addAndRemoveItems_Component, container)
            compare(layout.implicitWidth, 0)
            compare(layout.implicitHeight, 0)

            var rect0 = layoutItem_Component.createObject(layout)
            waitForItemPolished(layout)
            compare(layout.implicitWidth, 20)
            compare(layout.implicitHeight, 20)

            var rect1 = layoutItem_Component.createObject(layout)
            rect1.Layout.preferredWidth = 30;
            rect1.Layout.preferredHeight = 30;
            waitForItemPolished(layout)
            compare(layout.implicitWidth, 50)
            compare(layout.implicitHeight, 30)

            var col = columnLayoutItem_Component.createObject(layout)
            var rect2 = layoutItem_Component.createObject(col)
            rect2.Layout.fillHeight = true
            var rect3 = layoutItem_Component.createObject(col)
            rect3.Layout.fillHeight = true

            waitForItemPolished(layout)
            compare(layout.implicitWidth, 70)
            compare(col.implicitHeight, 40)
            compare(layout.implicitHeight, 40)

            rect3.destroy()
            wait(0)     // this will hopefully effectuate the destruction of the object
            waitForItemPolished(layout)

            col.destroy()
            wait(0)
            waitForItemPolished(layout)
            compare(layout.implicitWidth, 50)
            compare(layout.implicitHeight, 30)

            rect0.destroy()
            wait(0)
            waitForItemPolished(layout)
            compare(layout.implicitWidth, 30)
            compare(layout.implicitHeight, 30)

            rect1.destroy()
            wait(0)
            waitForItemPolished(layout)
            compare(layout.implicitWidth, 0)
            compare(layout.implicitHeight, 0)
        }

        Component {
            id: layout_alignment_Component
            RowLayout {
                spacing: 0
                Rectangle {
                    color: "red"
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                    Layout.fillHeight: true
                }
                Rectangle {
                    color: "red"
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                    // use default alignment
                }
                Rectangle {
                    color: "red"
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                    Layout.alignment: Qt.AlignTop
                }
                Rectangle {
                    color: "red"
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                    Layout.alignment: Qt.AlignVCenter
                }
                Rectangle {
                    color: "red"
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                    Layout.alignment: Qt.AlignBottom
                }
            }
        }

        function test_alignment()
        {
            var layout = layout_alignment_Component.createObject(container);
            layout.width = 100;
            layout.height = 40;
            waitForItemPolished(layout)
            compare(itemRect(layout.children[0]), [ 0,  0, 20, 40]);
            compare(itemRect(layout.children[1]), [20, 10, 20, 20]);
            compare(itemRect(layout.children[2]), [40,  0, 20, 20]);
            compare(itemRect(layout.children[3]), [60, 10, 20, 20]);
            compare(itemRect(layout.children[4]), [80, 20, 20, 20]);
            layout.destroy();
        }


        function buildLayout(layout, arrLayoutData) {
            for (let i = 0; i < arrLayoutData.length; i++) {
                let layoutItemDesc = arrLayoutData[i]
                let rect = layoutItem_Component.createObject(layout)
                for (let keyName in layoutItemDesc) {
                    rect.Layout[keyName] = layoutItemDesc[keyName]
                }
            }
        }

        function test_dynamicAlignment_data()
        {
            return [
            {
                tag: "simple",

                layout: {
                    type: "RowLayout",
                    items: [
                        {preferredWidth: 30, preferredHeight: 20, fillHeight: true},
                        {preferredWidth: 30, preferredHeight: 20},
                    ]
                },
                expectedGeometries: [
                    [ 0,  0, 30, 60],
                    [30, 20, 30, 20]
                ]
            },{
                tag: "valign",
                layout: {
                    type: "RowLayout",
                    items: [
                        {preferredWidth: 12, preferredHeight: 20, fillHeight: true},
                        {preferredWidth: 12, preferredHeight: 20},
                        {preferredWidth: 12, preferredHeight: 20, alignment: Qt.AlignTop},
                        {preferredWidth: 12, preferredHeight: 20, alignment: Qt.AlignVCenter},
                        {preferredWidth: 12, preferredHeight: 20, alignment: Qt.AlignBottom}
                    ]
                },
                expectedGeometries: [
                    [ 0,  0, 12, 60],
                    [12, 20, 12, 20],
                    [24,  0, 12, 20],
                    [36, 20, 12, 20],
                    [48, 40, 12, 20]
                ]
            },{
                tag: "halign",
                layout: {
                    type: "ColumnLayout",
                    items: [
                        {preferredWidth: 20, preferredHeight: 12, fillWidth: true},
                        {preferredWidth: 20, preferredHeight: 12},
                        {preferredWidth: 20, preferredHeight: 12, alignment: Qt.AlignLeft},
                        {preferredWidth: 20, preferredHeight: 12, alignment: Qt.AlignHCenter},
                        {preferredWidth: 20, preferredHeight: 12, alignment: Qt.AlignRight}
                    ]
                },
                expectedGeometries: [
                    [ 0,  0, 60, 12],
                    [ 0, 12, 20, 12],
                    [ 0, 24, 20, 12],
                    [20, 36, 20, 12],
                    [40, 48, 20, 12]
                ]
            }
            ]
        }

        function test_dynamicAlignment(data)
        {
            let layout
            switch (data.layout.type) {
            case "RowLayout":
                layout = createTemporaryObject(layout_rowLayout_Component, container)
                break
            case "ColumnLayout":
                layout = createTemporaryObject(layout_columnLayout_Component, container)
                break
            default:
                console.log("data.layout.type not recognized(" + data.layout.type + ")")
            }
            layout.spacing = 0
            buildLayout(layout, data.layout.items)
            layout.width = 60
            layout.height = 60      // divides in 1/2/3/4/5/6
            waitForItemPolished(layout)

            for (let i = 0; i < layout.children.length; ++i) {
                let itm = layout.children[i]
                compare(itemRect(itm), data.expectedGeometries[i])
            }
        }


        Component {
            id: layout_sizeHintNormalization_Component
            GridLayout {
                columnSpacing: 0
                rowSpacing: 0
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

        function test_sizeHintNormalization_data() {
            return [
                    { tag: "fallbackValues",  widthHints: [-1, -1, -1], implicitWidth: 42, expected:[0,42,Number.POSITIVE_INFINITY]},
                    { tag: "acceptZeroWidths",  widthHints: [0, 0, 0], implicitWidth: 42, expected:[0,0,0]},
                    { tag: "123",  widthHints: [1,2,3],  expected:[1,2,3]},
                    { tag: "132",  widthHints: [1,3,2],  expected:[1,2,2]},
                    { tag: "213",  widthHints: [2,1,3],  expected:[2,2,3]},
                    { tag: "231",  widthHints: [2,3,1],  expected:[1,1,1]},
                    { tag: "321",  widthHints: [3,2,1],  expected:[1,1,1]},
                    { tag: "312",  widthHints: [3,1,2],  expected:[2,2,2]},

                    { tag: "1i3",  widthHints: [1,-1,3], implicitWidth: 2,  expected:[1,2,3]},
                    { tag: "1i2",  widthHints: [1,-1,2], implicitWidth: 3,  expected:[1,2,2]},
                    { tag: "2i3",  widthHints: [2,-1,3], implicitWidth: 1,  expected:[2,2,3]},
                    { tag: "2i1",  widthHints: [2,-1,1], implicitWidth: 3,  expected:[1,1,1]},
                    { tag: "3i1",  widthHints: [3,-1,1], implicitWidth: 2,  expected:[1,1,1]},
                    { tag: "3i2",  widthHints: [3,-1,2], implicitWidth: 1,  expected:[2,2,2]},
                    ];
        }

        function test_sizeHintNormalization(data) {
            var layout = layout_sizeHintNormalization_Component.createObject(container);
            if (data.implicitWidth !== undefined) {
                layout.children[0].implicitWidth = data.implicitWidth
            }
            layout.children[0].Layout.minimumWidth = data.widthHints[0];
            layout.children[0].Layout.preferredWidth = data.widthHints[1];
            layout.children[0].Layout.maximumWidth = data.widthHints[2];
            waitForItemPolished(layout)
            var normalizedResult = [layout.Layout.minimumWidth, layout.implicitWidth, layout.Layout.maximumWidth]
            compare(normalizedResult, data.expected);
            layout.destroy();
        }

        Component {
            id: layout_sizeHint_Component
            RowLayout {
                property int implicitWidthChangedCount : 0
                onImplicitWidthChanged: { ++implicitWidthChangedCount }
                GridLayout {
                    columnSpacing: 0
                    rowSpacing: 0
                    Rectangle {
                        id: r1
                        color: "red"
                        implicitWidth: 1
                        implicitHeight: 1

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

        function test_sizeHint_data() {
            return [
                    { tag: "propagateNone",            layoutHints: [10, 20, 30], childHints: [11, 21, 31], expected:[10, 20, 30]},
                    { tag: "propagateMinimumWidth",    layoutHints: [-1, 20, 30], childHints: [10, 21, 31], expected:[10, 20, 30]},
                    { tag: "propagatePreferredWidth",  layoutHints: [10, -1, 30], childHints: [11, 20, 31], expected:[10, 20, 30]},
                    { tag: "propagateMaximumWidth",    layoutHints: [10, 20, -1], childHints: [11, 21, 30], expected:[10, 20, 30]},
                    { tag: "propagateAll",             layoutHints: [-1, -1, -1], childHints: [10, 20, 30], expected:[10, 20, 30]},
                    { tag: "propagateCrazy",           layoutHints: [-1, -1, -1], childHints: [40, 21, 30], expected:[30, 30, 30]},
                    { tag: "expandMinToExplicitPref",  layoutHints: [-1,  1, -1], childHints: [11, 21, 31], expected:[ 1,  1, 31]},
                    { tag: "expandMaxToExplicitPref",  layoutHints: [-1, 99, -1], childHints: [11, 21, 31], expected:[11, 99, 99]},
                    { tag: "expandAllToExplicitMin",   layoutHints: [99, -1, -1], childHints: [11, 21, 31], expected:[99, 99, 99]},
                    { tag: "expandPrefToExplicitMin",  layoutHints: [24, -1, -1], childHints: [11, 21, 31], expected:[24, 24, 31]},
                    { tag: "boundPrefToExplicitMax",   layoutHints: [-1, -1, 19], childHints: [11, 21, 31], expected:[11, 19, 19]},
                    { tag: "boundAllToExplicitMax",    layoutHints: [-1, -1,  9], childHints: [11, 21, 31], expected:[ 9,  9,  9]},

                    /**
                     * Test how fractional size hint values are rounded. Some hints are ceiled towards the closest integer.
                     * Note some of these tests are not authorative, but are here to demonstrate current behavior.
                     * To summarize, it seems to be:
                     *      - min: always ceiled
                     *      - pref:  Ceils only implicit (!) hints. Might also be ceiled if explicit
                              preferred size is less than implicit minimum size, but that's just a
                              side-effect of that preferred should never be less than minimum.
                              (tag "ceilShrinkMinToPref" below)
                     *      - max: never ceiled
                     */
                    { tag: "ceilImplicitMin",       layoutHints: [ -1,  -1,  -1], childHints: [ .1, 1.1, 9.1], expected:[  1,   2, 9.1]},
                    { tag: "ceilExplicitMin",       layoutHints: [1.1,  -1,  -1], childHints: [ .1, 2.1, 9.1], expected:[  2,   3, 9.1]},
                    { tag: "ceilImplicitMin2",      layoutHints: [ -1, 4.1,  -1], childHints: [ .1, 1.1, 9.1], expected:[  1, 4.1, 9.1]},
                    { tag: "ceilShrinkMinToPref",   layoutHints: [ -1, 2.1,  -1], childHints: [  5, 6.1, 8.1], expected:[  3,   3, 8.1]},
                    { tag: "ceilExpandMaxToPref",   layoutHints: [ -1, 6.1,  -1], childHints: [1.1, 3.1, 3.1], expected:[  2, 6.1, 6.1]},
                    ];
        }

        function itemSizeHints(item) {
            return [item.Layout.minimumWidth, item.implicitWidth, item.Layout.maximumWidth]
        }

        function test_sizeHint(data) {
            var layout = layout_sizeHint_Component.createObject(container)

            var grid = layout.children[0]
            grid.Layout.minimumWidth = data.layoutHints[0]
            grid.Layout.preferredWidth = data.layoutHints[1]
            grid.Layout.maximumWidth = data.layoutHints[2]

            var child = grid.children[0]
            if (data.implicitWidth !== undefined) {
                child.implicitWidth = data.implicitWidth
            }
            child.Layout.minimumWidth = data.childHints[0]
            child.Layout.preferredWidth = data.childHints[1]
            child.Layout.maximumWidth = data.childHints[2]

            waitForItemPolished(layout)
            var effectiveSizeHintResult = [layout.Layout.minimumWidth, layout.implicitWidth, layout.Layout.maximumWidth]
            compare(effectiveSizeHintResult, data.expected)
            layout.destroy()
        }

        function test_sizeHintPropagationCount() {
            var layout = layout_sizeHint_Component.createObject(container)
            var child = layout.children[0].children[0]

            child.Layout.minimumWidth = -1
            waitForItemPolished(layout)
            compare(itemSizeHints(layout), [0, 2, 3])
            child.Layout.preferredWidth = -1
            waitForItemPolished(layout)
            compare(itemSizeHints(layout), [0, 1, 3])
            child.Layout.maximumWidth = -1
            waitForItemPolished(layout)
            compare(itemSizeHints(layout), [0, 1, Number.POSITIVE_INFINITY])
            layout.Layout.maximumWidth = 1000
            waitForItemPolished(layout)
            compare(itemSizeHints(layout), [0, 1, 1000])
            layout.Layout.maximumWidth = -1
            waitForItemPolished(layout)
            compare(itemSizeHints(layout), [0, 1, Number.POSITIVE_INFINITY])

            layout.implicitWidthChangedCount = 0
            child.Layout.minimumWidth = 10
            waitForItemPolished(layout)
            compare(itemSizeHints(layout), [10, 10, Number.POSITIVE_INFINITY])
            compare(layout.implicitWidthChangedCount, 1)

            child.Layout.preferredWidth = 20
            waitForItemPolished(layout)
            compare(itemSizeHints(layout), [10, 20, Number.POSITIVE_INFINITY])
            compare(layout.implicitWidthChangedCount, 2)

            child.Layout.maximumWidth = 30
            waitForItemPolished(layout)
            compare(itemSizeHints(layout), [10, 20, 30])
            compare(layout.implicitWidthChangedCount, 2)

            child.Layout.maximumWidth = 15
            waitForItemPolished(layout)
            compare(itemSizeHints(layout), [10, 15, 15])
            compare(layout.implicitWidthChangedCount, 3)

            child.Layout.maximumWidth = 30
            waitForItemPolished(layout)
            compare(itemSizeHints(layout), [10, 20, 30])
            compare(layout.implicitWidthChangedCount, 4)

            layout.Layout.maximumWidth = 29
            waitForItemPolished(layout)
            compare(layout.Layout.maximumWidth, 29)
            layout.Layout.maximumWidth = -1
            compare(layout.Layout.maximumWidth, 30)

            layout.destroy()
        }

        Component {
            id: layout_change_implicitWidth_during_rearrange
            ColumnLayout {
                width: 100
                height: 20
                RowLayout {
                    spacing: 0
                    Rectangle {
                        Layout.fillHeight: true
                        Layout.fillWidth: false
                        implicitWidth: height
                        color: "red"
                    }
                    Rectangle {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        color: "blue"
                    }
                }
            }
        }

        function test_change_implicitWidth_during_rearrange() {
            var layout = layout_change_implicitWidth_during_rearrange.createObject(container)
            var red = layout.children[0].children[0]
            var blue = layout.children[0].children[1]
            waitForRendering(layout);
            tryCompare(red, 'width', 20)
            tryCompare(blue, 'width', 80)
            layout.height = 40
            tryCompare(red, 'width', 40)
            tryCompare(blue, 'width', 60)
            layout.destroy()
        }

        Component {
            id: layout_addIgnoredItem_Component
            RowLayout {
                spacing: 0
                Rectangle {
                    id: r
                }
            }
        }

        function test_addIgnoredItem()
        {
            var layout = layout_addIgnoredItem_Component.createObject(container)
            compare(layout.implicitWidth, 0)
            compare(layout.implicitHeight, 0)
            var r = layout.children[0]
            r.Layout.preferredWidth = 20
            r.Layout.preferredHeight = 30
            waitForItemPolished(layout)
            compare(layout.implicitWidth, 20)
            compare(layout.implicitHeight, 30)

            layout.destroy();
        }

        function test_stretchItem_data()
        {
            return [
                    { expectedWidth: 0},
                    { preferredWidth: 20, expectedWidth: 20},
                    { preferredWidth: 0, expectedWidth: 0},
                    { preferredWidth: 20, fillWidth: true, expectedWidth: 100},
                    { width: 20, fillWidth: true, expectedWidth: 100},
                    { width: 0, fillWidth: true, expectedWidth: 100},
                    { preferredWidth: 0, fillWidth: true, expectedWidth: 100},
                    { preferredWidth: 1, maximumWidth: 0, fillWidth: true, expectedWidth: 0},
                    { preferredWidth: 0, minimumWidth: 1, expectedWidth: 1},
                    ];
        }

        function test_stretchItem(data)
        {
            var layout = layout_rowLayout_Component.createObject(container)
            var r = layoutItem_Component.createObject(layout)
            // Reset previously relevant properties
            r.width = 0
            r.implicitWidth = 0
            compare(layout.implicitWidth, 0)

            if (data.preferredWidth !== undefined)
                r.Layout.preferredWidth = data.preferredWidth
            if (data.fillWidth !== undefined)
                r.Layout.fillWidth = data.fillWidth
            if (data.width !== undefined)
                r.width = data.width
            if (data.minimumWidth !== undefined)
                r.Layout.minimumWidth = data.minimumWidth
            if (data.maximumWidth !== undefined)
                r.Layout.maximumWidth = data.maximumWidth
            waitForItemPolished(layout)
            layout.width = 100

            compare(r.width, data.expectedWidth)

            layout.destroy();
        }

        function test_distribution_data()
        {
            return [
                {
                  tag: "one",
                  layout: {
                    type: "RowLayout",
                    items: [
                        {minimumWidth:  1, preferredWidth: 10, maximumWidth: 20, fillWidth: true},
                        {minimumWidth:  1, preferredWidth:  4, maximumWidth: 10, fillWidth: true},
                    ]
                  },
                  layoutWidth:     28,
                  expectedWidths: [20, 8]
                },{
                  tag: "two",
                  layout: {
                    type: "RowLayout",
                    items: [
                        {minimumWidth:  1, preferredWidth: 10, horizontalStretchFactor: 4, fillWidth: true},
                        {minimumWidth:  1, preferredWidth: 4,  horizontalStretchFactor: 1, fillWidth: true},
                      ]
                  },
                  layoutWidth:     28,
                  expectedWidths: [22, 6]
                },{
                    tag: "resize_to_0_width",
                    layout: {
                      type: "RowLayout",
                      items: [
                          {preferredWidth: 10, fillWidth: true},
                        ]
                    },
                    layoutWidth:     0,
                    expectedWidths: [0]
                },{
                  tag: "preferred_infinity",    // Do not crash/assert when the preferred size is infinity
                  layout: {
                    type: "RowLayout",
                    items: [
                        {minimumWidth:  10, preferredWidth: Number.POSITIVE_INFINITY, fillWidth: true},
                        {minimumWidth:  20, preferredWidth: Number.POSITIVE_INFINITY, fillWidth: true},
                      ]
                  },
                  layoutWidth:     31,      // Important that this is between minimum and preferred width of the layout.
                  expectedWidths: [10, 21]  // The result here does not have to be exact. (This
                                            // test is mostly concerned about not crashing).
                }
            ];
        }

        function test_distribution(data)
        {
            var layout = layout_rowLayout_Component.createObject(container)
            layout.spacing = 0
            buildLayout(layout, data.layout.items)
            waitForPolish(layout)
            layout.width = data.layoutWidth

            let actualWidths = []
            for (let i = 0; i < layout.children.length; i++) {
                actualWidths.push(layout.children[i].width)
            }
            compare(actualWidths, data.expectedWidths)
            layout.destroy();
        }

        function test_uniformCellSizes_data()
        {
            return [
                {
                  tag: "hor 9/3",
                  layout: {
                    type: "RowLayout",
                    items: [
                        {minimumWidth:  1, preferredWidth: 10, maximumWidth: 20, fillWidth: true},
                        {minimumWidth:  1, preferredWidth:  4, maximumWidth: 10, fillWidth: true},
                        {minimumWidth:  1, preferredWidth: 50, maximumWidth: 99, fillWidth: true}
                    ]
                  },
                  layoutWidth:     9,
                  expectedWidths: [3, 3, 3],
                  expectedPositions: [0, 3, 6]
                },
                {
                  tag: "hor 30/3",
                  layout: {
                    type: "RowLayout",
                    items: [
                        {minimumWidth:  1, preferredWidth: 10, maximumWidth: 20, fillWidth: true},
                        {minimumWidth:  1, preferredWidth:  4, maximumWidth: 10, fillWidth: true},
                        {minimumWidth:  1, preferredWidth: 50, maximumWidth: 99, fillWidth: true}
                    ]
                  },
                  layoutWidth:     30,
                  expectedWidths: [10, 10, 10]
                },
                {
                  tag: "hor 60/3",
                  layout: {
                    type: "RowLayout",
                    items: [
                        {minimumWidth:  1, preferredWidth: 10, maximumWidth: 20, fillWidth: true},
                        {minimumWidth:  1, preferredWidth:  4, maximumWidth: 10, fillWidth: true},
                        {minimumWidth:  1, preferredWidth: 50, maximumWidth: 99, fillWidth: true}
                    ]
                  },
                  layoutWidth:     60,
                  expectedWidths: [20, 10, 20],     // We are beyond the maximumWidth. of the middle item,
                  expectedPositions: [0, 20, 40]    // check that *cellSize* is still uniform
                                                    // (middle item will be left-aligned in the cell by default)
                },
                {
                  tag: "hor 66/3",
                  layout: {
                    type: "RowLayout",
                    items: [
                        {minimumWidth:  1, preferredWidth: 10, maximumWidth: 20, fillWidth: true},
                        {minimumWidth:  1, preferredWidth:  4, maximumWidth: 10, fillWidth: true},
                        {minimumWidth:  1, preferredWidth: 50, maximumWidth: 99, fillWidth: true}
                    ]
                  },
                  layoutWidth:     66,
                  expectedWidths: [20, 10, 22],
                  expectedPositions: [0, 22, 44]
                },
                {
                  tag: "ver 66/3",
                  layout: {
                    type: "ColumnLayout",
                    items: [
                        {minimumHeight:  1, preferredHeight: 10, maximumHeight: 20, fillHeight: true},
                        {minimumHeight:  1, preferredHeight:  4, maximumHeight: 10, fillHeight: true},
                        {minimumHeight:  1, preferredHeight: 50, maximumHeight: 99, fillHeight: true}
                    ]
                  },
                  layoutHeight:     66,
                  expectedHeights: [20, 10, 22],
                    // If items are too small to fit the cell, they have a default alignment of
                    // Qt::AlignLeft | Qt::AlignVCenter
                  expectedPositions: [1, 22+6, 44]
                }
            ];
        }

        function test_uniformCellSizes(data)
        {
            let layout = LayoutHelpers.buildLayout(data.layout, testCase)
            let isHorizontal = data.hasOwnProperty("expectedWidths")
            layout.spacing = 0
            layout.uniformCellSizes = true
            waitForPolish(layout)
            if (data.hasOwnProperty('layoutWidth')) {
                layout.width = data.layoutWidth
            }
            if (data.hasOwnProperty('layoutHeight')) {
                layout.height = data.layoutHeight
            }

            let expectedSizes = (isHorizontal ? data.expectedWidths : data.expectedHeights)
            let actualSizes = []
            let i = 0
            for (i = 0; i < layout.children.length; i++) {
                let item = layout.children[i]
                actualSizes.push(isHorizontal ? item.width : item.height)
            }
            compare(actualSizes, expectedSizes)

            if (data.hasOwnProperty('expectedPositions')) {
                let actualPositions = []
                for (i = 0; i < layout.children.length; i++) {
                    let item = layout.children[i]
                    actualPositions.push(isHorizontal ? item.x : item.y)
                }
                compare(actualPositions, data.expectedPositions)
            }
        }

        Component {
            id: uniformCellSizes_QML_Component
            RowLayout {
                spacing: 0
                uniformCellSizes: true
                Rectangle {
                    implicitWidth: 1
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    color: "red"
                }
                Rectangle {
                    implicitWidth: 2
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    color: "blue"
                }
            }
        }

        function test_uniformCellSizes_QML(data)
        {
            var layout = createTemporaryObject(uniformCellSizes_QML_Component, testCase)
            layout.width  = 40
            layout.height = 20
            let expectedWidths = [20, 20]
            let actualWidths = [layout.children[0].width, layout.children[1].width]
            compare(actualWidths, expectedWidths)
        }


        Component {
            id: layout_alignToPixelGrid_Component
            RowLayout {
                spacing: 2
                Rectangle {
                    implicitWidth: 10
                    implicitHeight: 10
                    Layout.alignment: Qt.AlignVCenter
                }
                Rectangle {
                    implicitWidth: 10
                    implicitHeight: 10
                    Layout.alignment: Qt.AlignVCenter
                }
            }
        }
        function test_alignToPixelGrid()
        {
            var layout = layout_alignToPixelGrid_Component.createObject(container)
            layout.width  = 21
            layout.height = 21
            var r0 = layout.children[0]
            compare(r0.x, 0) // 0.0
            compare(r0.y, 6) // 5.5
            var r1 = layout.children[1]
            compare(r1.x, 12) // 11.5
            compare(r1.y, 6) // 5.5
            layout.destroy();
        }

        Component {
            id: test_distributeToPixelGrid_Component
            RowLayout {
                spacing: 0
            }
        }

        function test_distributeToPixelGrid_data() {
            return [
                    { tag: "narrow",  spacing: 0, width: 60, hints: [{pref: 50}, {pref: 20}, {pref: 70}] },
                    { tag: "belowPreferred",  spacing: 0, width: 130, hints: [{pref: 50}, {pref: 20}, {pref: 70}]},
                    { tag: "belowPreferredWithSpacing", spacing: 10, width: 130, hints: [{pref: 50}, {pref: 20}, {pref: 70}]},
                    { tag: "abovePreferred",  spacing: 0, width: 150, hints: [{pref: 50}, {pref: 20}, {pref: 70}]},
                    { tag: "stretchSomethingToMaximum",  spacing: 0, width: 240, hints: [{pref: 50}, {pref: 20}, {pref: 70}],
                      expected: [90, 60, 90] },
                    { tag: "minSizeHasFractions",  spacing: 2, width: 33 + 4, hints: [{min: 10+1/3}, {min: 10+1/3}, {min: 10+1/3}],
                      /*expected: [11, 11, 11]*/ },     /* verify that nothing gets allocated a size smaller than its minimum */
                    { tag: "maxSizeHasFractions",  spacing: 2, width: 271 + 4, hints: [{max: 90+1/3}, {max: 90+1/3}, {max: 90+1/3}],
                      /*expected: [90, 90, 90]*/ },     /* verify that nothing gets allocated a size larger than its maximum */
                    { tag: "fixedSizeHasFractions",  spacing: 2, width: 31 + 4, hints: [{min: 10+1/3, max: 10+1/3}, {min: 10+1/3, max: 10+1/3}, {min: 10+1/3, max: 10+1/3}],
                      /*expected: [11, 11, 11]*/ },     /* verify that nothing gets allocated a size smaller than its minimum */
                    { tag: "481", spacing: 0, width: 481,
                      hints: [{min:0, pref:0, max:999}, {min:0, pref:0, max: 999}, {min: 0, pref: 0, max:0}],
                      expected: [241, 240, 0] },
                    { tag: "theend", spacing: 1, width: 18,
                      hints: [{min: 10, pref: 10, max:10}, {min:3, pref:3.33}, {min:2, pref:2.33}],
                      expected: [10, 4, 2] },
                    { tag: "theend2",  spacing: 1, width: 18,
                      hints: [{min: 10, pref: 10, max:10}, {min:3, pref:3.33}, {min:2.33, pref:2.33}],
                      expected: [10, 3, 3] },
                    { tag: "43",  spacing: 0, width: 43,
                      hints: [{min: 10, pref: 10, max:10}, {min:10, pref:30.33}, {min:2.33, pref:2.33}],
                      expected: [10, 30, 3] },
                    { tag: "40",  spacing: 0, width: 40,
                      hints: [{min: 10, pref: 10, max:10}, {min:10, pref:30.33}, {min:2.33, pref:2.33}],
                      expected: [10, 27, 3] },
                    { tag: "roundingAccumulates1",  spacing: 0, width: 50,
                      hints: [{pref: 10, max:30.3},
                              {min:2.3, pref:2.3}, {min:2.3, pref:2.3}, {min:2.3, pref:2.3}, {min:2.3, pref:2.3}, {min:2.3, pref:2.3},
                              {min:2.3, pref:2.3}, {min:2.3, pref:2.3}, {min:2.3, pref:2.3}, {min:2.3, pref:2.3}, {min:2.3, pref:2.3},
                              {pref: 10, max:30.3}],
                      expected: [10,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   10] },
                    { tag: "roundingAccumulates2",  spacing: 0, width: 60,
                      hints: [{pref: 20, max:30.3},
                              {min:2.3, pref:2.3}, {min:2.3, pref:2.3}, {min:2.3, pref:2.3}, {min:2.3, pref:2.3}, {min:2.3, pref:2.3},
                              {min:2.3, pref:2.3}, {min:2.3, pref:2.3}, {min:2.3, pref:2.3}, {min:2.3, pref:2.3}, {min:2.3, pref:2.3},
                              {pref: 20, max:30.3}],
                      expected: [15,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   15] },
                    ];
        }

        function test_distributeToPixelGrid(data)
        {
            // CONFIGURATION
            var layout = test_distributeToPixelGrid_Component.createObject(container)
            layout.spacing = data.spacing
            layout.width  = data.width
            layout.height = 10

            var hints = data.hints
            var i;
            var n = hints.length
            for (i = 0; i < n; ++i) {
                var rect = layoutItem_Component.createObject(layout)
                rect.Layout.fillWidth = true
                var h = hints[i]
                rect.Layout.minimumWidth = h.hasOwnProperty('min') ? h.min : 10
                if (h.hasOwnProperty('pref'))
                    rect.Layout.preferredWidth = h.pref
                rect.Layout.maximumWidth = h.hasOwnProperty('max') ? h.max : 90
            }

            var kids = layout.children

            waitForRendering(layout)

            var sum = (n - 1) * layout.spacing
            // TEST
            for (i = 0; i < n; ++i) {
                compare(kids[i].x % 1, 0)           // checks if position is a whole integer
                // check if width is a whole integer (unless there are constraints preventing it from stretching)
                verify(kids[i].width % 1 == 0
                       || Math.floor(kids[i].Layout.maximumWidth) < kids[i].width
                       || layout.width < layout.Layout.maximumWidth + 1)
                // verify if the items are within the size constraints as specified
                verify(kids[i].width >= kids[i].Layout.minimumWidth)
                verify(kids[i].width <= kids[i].Layout.maximumWidth)
                if (data.hasOwnProperty('expected'))
                    compare(kids[i].width,  data.expected[i])
                sum += kids[i].width
            }
            fuzzyCompare(sum, layout.width, 1)

            layout.destroy();
        }



        Component {
            id: layout_deleteLayout
            ColumnLayout {
                property int dummyproperty: 0   // yes really - its needed
                RowLayout {
                    Text { text: "label1" }     // yes, both are needed
                    Text { text: "label2" }
                }
            }
        }

        function test_destroyLayout()
        {
            var layout = layout_deleteLayout.createObject(container)
            layout.children[0].children[0].visible = true
            layout.visible = false
            layout.destroy()    // Do not crash
        }

        function test_destroyImplicitInvisibleLayout()
        {
            var root = rectangle_Component.createObject(container)
            root.visible = false
            var layout = layout_deleteLayout.createObject(root)
            layout.visible = true
            // at this point the layout is still invisible because root is invisible
            layout.destroy()
            // Do not crash when destructing the layout
            waitForRendering(container)      // should ideally call gc(), but does not work
            root.destroy()
        }

        function test_sizeHintWithHiddenChildren(data) {
            var layout = layout_sizeHint_Component.createObject(container)
            var grid = layout.children[0]
            var child = grid.children[0]

            // Implicit sizes are not affected by the visibility of the parent layout.
            // This is in order for the layout to know the preferred size it should show itself at.
            compare(grid.visible, true)     // LAYOUT SHOWN
            compare(grid.implicitWidth, 2);
            child.visible = false
            waitForItemPolished(layout)
            compare(grid.implicitWidth, 0);
            child.visible = true
            waitForItemPolished(layout)
            compare(grid.implicitWidth, 2);

            grid.visible = false            // LAYOUT HIDDEN
            waitForItemPolished(layout)
            compare(grid.implicitWidth, 2);
            child.visible = false
            expectFail('', 'If GridLayout is hidden, GridLayout is not notified when child is explicitly hidden')
            waitForItemPolished(grid)
            compare(grid.implicitWidth, 0);
            child.visible = true
            waitForItemPolished(grid)
            compare(grid.implicitWidth, 2);

            layout.destroy();
        }

        Component {
            id: row_sizeHint_Component
            Row {
                Rectangle {
                    id: r1
                    color: "red"
                    width: 2
                    height: 20
                }
            }
        }

        function test_sizeHintWithHiddenChildrenForRow(data) {
            var row = row_sizeHint_Component.createObject(container)
            var child = row.children[0]
            compare(row.visible, true)     // POSITIONER SHOWN
            compare(row.implicitWidth, 2);
            child.visible = false
            tryCompare(row, 'implicitWidth', 0);
            child.visible = true
            tryCompare(row, 'implicitWidth', 2);

            row.visible = false            // POSITIONER HIDDEN
            compare(row.implicitWidth, 2);
            child.visible = false
            expectFail('', 'If Row is hidden, Row is not notified when child is explicitly hidden')
            compare(row.implicitWidth, 0);
            child.visible = true
            compare(row.implicitWidth, 2);
        }

        Component {
            id: sizeHintBindingLoopComp
            Item {
                id: root
                anchors.fill: parent
                property var customWidth: 100
                RowLayout {
                    id: col
                    Item {
                        id: item
                        implicitHeight: 80
                        implicitWidth: Math.max(col2.implicitWidth, root.customWidth + 20)
                        ColumnLayout {
                            id: col2
                            width: parent.width
                            Item {
                                id: rect
                                implicitWidth: root.customWidth
                                implicitHeight: 80
                            }
                        }
                    }
                }
            }
        }

        function test_sizeHintBindingLoopIssue() {
            var item = createTemporaryObject(sizeHintBindingLoopComp, container)
            waitForRendering(item)
            item.customWidth += 10
            waitForRendering(item)
            verify(!LayoutSetup.bindingLoopDetected, "Detected binding loop")
            LayoutSetup.resetBindingLoopDetectedFlag()
        }

        Component {
            id: polishLayoutItemComp
            Item {
                anchors.fill: parent
                implicitHeight: contentLayout.implicitHeight
                implicitWidth: contentLayout.implicitWidth
                property alias textLayout: contentLayout
                RowLayout {
                    width: parent.width
                    height: parent.height
                    ColumnLayout {
                        id: contentLayout
                        Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
                        Layout.maximumWidth: 200
                        Repeater {
                            model: 2
                            Text {
                                Layout.fillWidth: true
                                text: "This is a long text causing line breaks to show the bug."
                                wrapMode: Text.Wrap
                            }
                        }
                        Item {
                            Layout.fillHeight: true
                        }
                    }
                }
            }
        }

        function test_polishLayoutItemIssue() {
            var rootItem = createTemporaryObject(polishLayoutItemComp, container)
            waitForRendering(rootItem)
            var textItem = rootItem.textLayout.children[1]
            verify(textItem.y >= rootItem.textLayout.children[0].height)
        }

        Component {
            id: rearrangeNestedLayouts_Component
            RowLayout {
                id: layout
                anchors.fill: parent
                width: 200
                height: 20
                RowLayout {
                    id: row
                    spacing: 0

                    Rectangle {
                        id: fixed
                        color: 'red'
                        implicitWidth: 20
                        implicitHeight: 20
                    }
                    Rectangle {
                        id: filler
                        color: 'grey'
                        Layout.fillWidth: true
                        implicitHeight: 20
                    }
                }
            }
        }

        function test_rearrangeNestedLayouts()
        {
            var layout = rearrangeNestedLayouts_Component.createObject(container)
            var fixed = layout.children[0].children[0]
            var filler = layout.children[0].children[1]

            compare(itemRect(fixed),  [0,0,20,20])
            compare(itemRect(filler), [20,0,180,20])

            fixed.implicitWidth = 100
            waitForRendering(layout)
            wait(0);    // Trigger processEvents() (allow LayoutRequest to be processed)
            compare(itemRect(fixed),  [0,0,100,20])
            compare(itemRect(filler), [100,0,100,20])
        }

        Component {
            id: rearrangeFixedSizeLayout_Component
            RowLayout {
                id: layout
                width: 200
                height: 20
                spacing: 0
                RowLayout {
                    id: row
                    spacing: 0
                    Rectangle {
                        id: r0
                        color: 'red'
                        implicitWidth: 20
                        implicitHeight: 20
                    }
                    Rectangle {
                        id: r1
                        color: 'grey'
                        implicitWidth: 80
                        implicitHeight: 20
                    }
                }
                ColumnLayout {
                    id: row2
                    spacing: 0
                    Rectangle {
                        id: r2_0
                        color: 'blue'
                        Layout.fillWidth: true
                        implicitWidth: 100
                        implicitHeight: 20
                    }
                }
            }
        }
        function test_rearrangeFixedSizeLayout()
        {
            var layout = createTemporaryObject(rearrangeFixedSizeLayout_Component, testCase)
            var row = layout.children[0]
            var r0 = row.children[0]
            var r1 = row.children[1]

            waitForRendering(layout)
            compare(itemRect(r0),  [0,0,20,20])
            compare(itemRect(r1), [20,0,80,20])

            // just swap their widths. The layout should keep the same size
            r0.implicitWidth = 80
            r1.implicitWidth = 20
            waitForRendering(layout)
            // even if the layout did not change size, it should rearrange its children
            compare(itemRect(row), [0,0, 100, 20])
            compare(itemRect(r0),  [0,0,80,20])
            compare(itemRect(r1), [80,0,20,20])
        }

        Component {
            id: rearrangeInvalidatedChildInNestedLayout
            ColumnLayout {
                anchors.fill: parent
                RowLayout {
                    spacing: 0
                    Text {
                        Layout.preferredWidth: 50
                        text: "Text Text Text"
                        wrapMode: Text.WordWrap
                    }
                    Rectangle {
                        color: "red";
                        Layout.preferredWidth: 50
                        Layout.preferredHeight: 20
                    }
                    Rectangle {
                        color: "green"
                        Layout.preferredHeight: 20
                        Layout.fillWidth: true
                    }
                }
            }
        }

        function test_rearrangeInvalidatedChildInNestedLayout() {
            let layout = rearrangeInvalidatedChildInNestedLayout.createObject(container)
            waitForRendering(layout)

            let item1 = layout.children[0].children[0]
            let item2 = layout.children[0].children[1]
            let item3 = layout.children[0].children[2]

            compare(item1.width, 50)
            compare(item2.width, 50)
            compare(item3.width, 100)
        }

        Component {
            id: changeChildrenOfHiddenLayout_Component
            RowLayout {
                property int childCount: 1
                Repeater {
                    model: parent.childCount
                    Text {
                        text: 'Just foo it'
                    }
                }
            }
        }
        function test_changeChildrenOfHiddenLayout()
        {
            var layout = changeChildrenOfHiddenLayout_Component.createObject(container)
            var child = layout.children[0]
            waitForRendering(layout)
            layout.visible = false
            waitForRendering(layout)
            // Remove and add children to the hidden layout..
            layout.childCount = 0
            waitForRendering(layout)
            layout.childCount = 1
            waitForRendering(layout)
            layout.destroy()
        }


        function test_defaultPropertyAliasCrash() {
            var containerUserComponent = Qt.createComponent("rowlayout/ContainerUser.qml");
            compare(containerUserComponent.status, Component.Ready);

            var containerUser = containerUserComponent.createObject(testCase);
            verify(containerUser);

            // Shouldn't crash.
            containerUser.destroy();
        }

        function test_defaultPropertyAliasCrashAgain() {
            var containerUserComponent = Qt.createComponent("rowlayout/ContainerUser2.qml");
            compare(containerUserComponent.status, Component.Ready);

            var containerUser = createTemporaryObject(containerUserComponent, testCase);
            verify(containerUser);

            // Shouldn't crash upon destroying containerUser.
        }

        /*
            Tests that a layout-managed item that sets layer.enabled to true
            still renders something. This is a simpler test case that only
            reproduces the issue when the layout that manages it is made visible
            after component completion, but QTBUG-63269 has a more complex example
            where this (setting visible to true afterwards) isn't necessary.
        */
        function test_layerEnabled() {
            var component = Qt.createComponent("rowlayout/LayerEnabled.qml");
            compare(component.status, Component.Ready);

            var rootRect = createTemporaryObject(component, container);
            verify(rootRect);
            rootRect.layout.visible = true;
            waitForRendering(rootRect.layout)
            compare(rootRect.item1.width, 100)
        }

        //---------------------------
        // Layout with negative size
        Component {
            id: negativeSize_Component
            Item {
                id: rootItem
                width: 0
                height: 0
                // default width x height: (0 x 0)
                RowLayout {
                    spacing: 0
                    anchors.fill: parent
                    anchors.leftMargin: 1   // since parent size == (0 x 0), it causes layout size
                    anchors.bottomMargin: 1 // to become (-1, -1)
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }
                }
            }
        }

        function test_negativeSize() {
            let rootItem = createTemporaryObject(negativeSize_Component, container)
            let rowLayout = rootItem.children[0]
            let item = rowLayout.children[0]

            const arr = [7, 1, 7, 0]
            arr.forEach((n) => {
                                rootItem.width = n
                                rootItem.height = n

                                // n === 0 is special: It will cause the layout to have a
                                // negative size. In this case it will simply not rearrange its
                                // child (and leave it at its previous size, 6)
                                const expectedItemExtent = n === 0 ? 6 : n - 1

                                compare(item.width, expectedItemExtent)
                                compare(item.height, expectedItemExtent)
                               });
        }


//---------------------------
        Component {
            id: rowlayoutWithTextItems_Component
            RowLayout {
                Text {
                    Layout.fillWidth: true
                    text: "OneWord"
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                }
                Text {
                    Layout.fillWidth: true
                    text: "OneWord"
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                }
            }
        }

        // QTBUG-73683
        function test_rowlayoutWithTextItems() {
            var layout = createTemporaryObject(rowlayoutWithTextItems_Component, container)
            waitForRendering(layout)
            layout.width = layout.width - 2     // set the size to be smaller than its "minimum size"
            waitForRendering(layout)    // do not exit before all warnings have been received

            // DO NOT CRASH due to stack overflow (or loop endlessly due to updatePolish()/polish() loop)
        }

        Component {
            id: layout_dependentWidth_QTBUG_87253_Component

            RowLayout {
                anchors.fill: parent;

                RowLayout {
                    spacing: 10

                    Text {
                        id: btnOPE
                        text: qsTr("Ok")
                        Layout.fillWidth: true
                        Layout.preferredWidth: (parent.width - 20) / 2
                    }

                    Text {
                        id: btnSeeChanged
                        text: qsTr("Not Ok");
                        Layout.fillWidth: true
                        Layout.preferredWidth: (parent.width - 20) / 2
                    }
                }
            }
        }

        function test_dependentWidth_QTBUG_87253()
        {
            var layout = createTemporaryObject(layout_dependentWidth_QTBUG_87253_Component, container)
            // Do not crash
            waitForRendering(layout)
        }

        //---------------------------
        Component {
            id: rowlayoutWithRectangle_Component
            RowLayout {
                property alias spy : signalSpy
                Rectangle {
                    color: "red"
                    implicitWidth: 10
                    implicitHeight: 10
                }
                SignalSpy {
                    id: signalSpy
                    target: parent
                    signalName: "implicitWidthChanged"
                }
            }
        }

        // QTBUG-93988
        function test_ensurePolished() {
            var layout = createTemporaryObject(rowlayoutWithRectangle_Component, container)
            compare(layout.spy.count, 1)
            waitForRendering(layout)
            compare(layout.implicitWidth, 10)
            var r0 = layout.children[0]

            r0.implicitWidth = 42
            compare(layout.spy.count, 1)    // Not yet updated, awaiting PolishEvent...
            layout.ensurePolished()
            compare(layout.spy.count, 2)
            compare(layout.implicitWidth, 42)
        }

        //---------------------------
        Component {
            id: rowlayoutCausesBindingLoop_Component
            Item {
                id: root
                width: 100
                height: 100
                property real maxWidth : Math.max(header.implicitWidth, content.implicitWidth)

                RowLayout {
                    id: header
                    y: 0

                    Rectangle {
                        color: "red"
                        implicitWidth: 10
                        implicitHeight: 10
                    }
                }
                Rectangle {
                    id: content
                    y: 10
                    implicitWidth: 42
                    implicitHeight: 10
                    color: Qt.rgba(root.maxWidth/66, 0, 1, 1)
                }
            }
        }
        function test_bindingLoop() {
            var rootItem = createTemporaryObject(rowlayoutCausesBindingLoop_Component, container)
            waitForRendering(rootItem)
            var header = rootItem.children[0]
            var content = rootItem.children[1]
            var rect = header.children[0]
            rect.implicitWidth = 20
            content.implicitWidth = 66
            waitForItemPolished(header)
            compare(rootItem.maxWidth, 66)

            // Should not trigger a binding loop
            verify(!LayoutSetup.bindingLoopDetected, "Detected binding loop")
            LayoutSetup.resetBindingLoopDetectedFlag()
        }


        //---------------------------
        // QTBUG-111792
        Component {
            id: rowlayoutCrashes_Component
            RowLayout {
                spacing: 5
                Rectangle {
                    color: "red"
                    implicitWidth: 10
                    implicitHeight: 10
                }
                Rectangle {
                    color: "green"
                    implicitWidth: 10
                    implicitHeight: 10
                }
            }
        }

        function test_dontCrashAfterDestroyingChildren_data() {
            return [
                        { tag: "setWidth", func: function (layout) { layout.width = 42 } },
                        { tag: "setHeight", func: function (layout) { layout.height = 42 } },
                        { tag: "getImplicitWidth", func: function (layout) { let x = layout.implicitWidth } },
                        { tag: "getImplicitHeight", func: function (layout) { let x = layout.implicitHeight } },
                    ]
        }

        function test_dontCrashAfterDestroyingChildren(data) {
            var layout = createTemporaryObject(rowlayoutCrashes_Component, container)
            waitForRendering(layout)
            compare(layout.implicitWidth, 25)
            layout.children[0].destroy()    // deleteLater()
            wait(0)                         // process the scheduled delete and actually invoke the dtor
            data.func(layout)               // call a function that might ultimately access the deleted item (but shouldn't)
        }

        //---------------------------
        // Default layout size policy
        Component {
            id: defaultLayoutComp
            Item {
                id: rootItem
                width: 110
                height: 100
                // Check default layout size policy
                RowLayout {
                    spacing: 0
                    anchors.fill: parent
                    // Rectangle item - SizePolicy { Horizontal: Fixed;  Vertical: Fixed }
                    Rectangle {}
                    // Button item - SizePolicy { Horizontal: Preferred; Vertical: Fixed }
                    Button {}
                    // Frame item - SizePolicy { Horizontal: Preferred; Vertical: Preferred }
                    Frame {}
                }
            }
        }

        function test_defaultLayoutSize() {
            let rootItem = createTemporaryObject(defaultLayoutComp, container)
            waitForRendering(rootItem)

            let rowLayout = rootItem.children[0]

            // Test default size policy disabled by default
            compare(LayoutSetup.useDefaultSizePolicy, false)

            let defaultButtonWidth = rowLayout.children[1].width
            let defaultFrameWidth = rowLayout.children[2].width

            // Enable attached properties for items and check its size
            {
                rowLayout.children[0].Layout.fillWidth = true
                rowLayout.children[0].Layout.fillHeight = true
                rowLayout.children[1].Layout.fillWidth = true
                rowLayout.children[1].Layout.fillHeight = true
                rowLayout.children[2].Layout.fillWidth = true
                rowLayout.children[2].Layout.fillHeight = true
                waitForRendering(rowLayout)
                let isAbovePreferred = rowLayout.width >= rowLayout.implicitWidth
                // Removing rectangle as width & implicitWidth be zero always, which makes validation of no use
                for (let i = 1; i < rowLayout.children.length; i++) {
                    compare(rowLayout.children[i].width >= rowLayout.children[i].implicitWidth, isAbovePreferred)
                    compare(rowLayout.children[i].height, rowLayout.height)
                }
            }

            // Destroy existing object
            rootItem.destroy()

            // Enable default size policy
            LayoutSetup.useDefaultSizePolicy = true
            compare(LayoutSetup.useDefaultSizePolicy, true)
            rootItem = createTemporaryObject(defaultLayoutComp, container)
            waitForRendering(rootItem)
            rowLayout = rootItem.children[0]

            // The default size policy would stretch button and frame accordingly
            {
                verify(Math.abs(rowLayout.width - (rowLayout.children[0].width + rowLayout.children[1].width + rowLayout.children[2].width)) <= 1)
                compare(rowLayout.children[1].width < rowLayout.children[2].width, rowLayout.children[1].implicitWidth < rowLayout.children[2].implicitWidth)
                compare(rowLayout.children[1].height, rowLayout.children[1].implicitHeight)
                compare(rowLayout.children[2].height, rowLayout.height)
            }

            // Change the width and height of the root item to see layout size change
            // Since default size policy for button and frame are Preferred, these items should
            // stretch
            {
                let szDefaultButtonWidth = rowLayout.children[1].width
                let szDefaultFrameWidth = rowLayout.children[2].width

                rootItem.width = 210
                rootItem.height = 200
                waitForRendering(rootItem)
                verify(rowLayout.children[1].width > szDefaultButtonWidth)
                compare(rowLayout.children[1].height, rowLayout.children[1].implicitHeight)
                verify(rowLayout.children[2].width > szDefaultFrameWidth)
                compare(rowLayout.children[2].height, rowLayout.height)
            }

            // Disable size policies through attached properties and check item size
            {
                rowLayout.children[1].Layout.fillWidth = false
                rowLayout.children[2].Layout.fillWidth = false
                rowLayout.children[2].Layout.fillHeight = false
                waitForRendering(rowLayout)
                compare(rowLayout.children[1].width, defaultButtonWidth)
                compare(rowLayout.children[2].width, defaultFrameWidth)
                for (let index = 1; index < rowLayout.children.length; index++)
                    compare(rowLayout.children[index].height, rowLayout.children[index].implicitHeight)
            }

            rootItem.destroy()
        }
    }
}
