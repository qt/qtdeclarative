// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.6
import QtTest 1.0
import QtQuick.Layouts

Item {
    id: container
    width: 200
    height: 200
    TestCase {
        id: testCase
        name: "Tests_LayoutProxy"
        when: windowShown
        width: parent.width
        height: parent.height

        Component {
            id: layout_proxy_Component
            Item {
                anchors.fill: container

                property var rect1: Rectangle {
                    id: redRectanlge
                    parent: container
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: 0
                    property var itemRect: [mapToItem(container, Qt.point(0, 0)).x,
                                            mapToItem(container, Qt.point(0, 0)).y,
                                            width, height]
                    color: "red"
                }


                property var rect2: Rectangle {
                    id: blueRectanlge
                    parent: container
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: 0
                    property var itemRect: [mapToItem(container, Qt.point(0, 0)).x,
                                            mapToItem(container, Qt.point(0, 0)).y,
                                            width, height]
                    color: "blue"
                }

                property var layout1: RowLayout {
                    parent: container
                    anchors.fill: parent
                    spacing: 0
                    LayoutItemProxy { target: redRectanlge }
                    LayoutItemProxy { target: blueRectanlge }
                }

                property var layout2: ColumnLayout {
                    parent: container
                    anchors.fill: parent
                    spacing: 0
                    LayoutItemProxy { target: redRectanlge }
                    LayoutItemProxy { target: blueRectanlge }
                }
            }
        }

        function test_Proxy_simple()
        {
            var item = createTemporaryObject(layout_proxy_Component, container);

            item.layout2.visible = false
            item.layout1.visible = true

            tryCompare(item.rect1, "itemRect", [ 0,   0, 100, 200])
            tryCompare(item.rect2, "itemRect", [ 100, 0, 100, 200])

            item.layout1.visible = false
            item.layout2.visible = true

            tryCompare(item.rect1, "itemRect", [ 0,   0, 200, 100])
            tryCompare(item.rect2, "itemRect", [ 0, 100, 200, 100])

            item.layout1.visible = true
            item.layout2.visible = false

            tryCompare(item.rect1, "itemRect", [ 0,   0, 100, 200])
            tryCompare(item.rect2, "itemRect", [ 100, 0, 100, 200])

        }

        function test_Proxy_layout_destruction1()
        {
            var item = createTemporaryObject(layout_proxy_Component, container);

            item.layout1.visible = true
            item.layout2.visible = false

            tryCompare(item.rect1, "itemRect", [ 0,   0, 100, 200])
            tryCompare(item.rect2, "itemRect", [ 100, 0, 100, 200])

            item.layout1.visible = false
            item.layout2.visible = true

            tryCompare(item.rect1, "itemRect", [ 0,   0, 200, 100])
            tryCompare(item.rect2, "itemRect", [ 0, 100, 200, 100])

            item.layout2.destroy() //destroy the layout that has control
            wait(0) // process the scheduled delete and actually invoke the dtor
            compare(item.layout2, null)
            item.layout1.visible = true //check that the other one still works

            tryCompare(item.rect1, "itemRect", [ 0,   0, 100, 200])
            tryCompare(item.rect2, "itemRect", [ 100, 0, 100, 200])
        }

        function test_Proxy_layout_destruction2()
        {
            var item = createTemporaryObject(layout_proxy_Component, container);

            item.layout1.visible = false
            item.layout2.visible = false

            //destroy both layouts while none has control
            item.layout1.destroy()
            item.layout2.destroy()
            wait(0) // process the scheduled delete and actually invoke the dtor
            //all layouts should be gone now
            compare(item.layout1, null)
            compare(item.layout2, null)
            //but the rectangles should still be here
            verify(item.rect1 !== null)
            verify(item.rect2 !== null)
        }

        function test_Proxy_layout_destruction3()
        {
            var item = createTemporaryObject(layout_proxy_Component, container);

            item.layout1.visible = true
            item.layout2.visible = true

            //destroy both layouts while both have control
            item.layout1.destroy()
            item.layout2.destroy()
            wait(0) // process the scheduled delete and actually invoke the dtor
            //all layouts should be gone now
            compare(item.layout1, null)
            compare(item.layout2, null)
            //but the rectangles should still be here
            verify(item.rect1 !== null)
            verify(item.rect2 !== null)
        }

        function test_Proxy_layout_destruction_of_targets()
        {
            var item = createTemporaryObject(layout_proxy_Component, container);

            item.layout1.visible = true
            item.layout2.visible = false

            //destroy a rectangle just to see if the proxy crashes
            item.rect1.destroy()
            wait(0) // process the scheduled delete and actually invoke the dtor
            compare(item.rect1, null)

            //the proxy still has the size of the item and is still there
            tryCompare(item.layout1.children[0], "x", 0)
            tryCompare(item.layout1.children[0], "y", 0)
            tryCompare(item.layout1.children[0], "width", 100)
            tryCompare(item.layout1.children[0], "height", 200)
            //the second item is still here
            tryCompare(item.rect2, "itemRect", [ 100, 0, 100, 200])
            //the most important thing is that it does not crash

        }

        Component {
            id: layout_proxy_Component_Three
            Item {
                anchors.fill: container
                property var rect1: Rectangle {
                    id: redRectanlge
                    parent: container
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: 0
                    property var itemRect: [mapToItem(container, Qt.point(0, 0)).x,
                                            mapToItem(container, Qt.point(0, 0)).y,
                                            width, height]
                    color: "red"
                }

                property var rect2: Rectangle {
                    id: blueRectanlge
                    parent: container
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: 0
                    property var itemRect: [mapToItem(container, Qt.point(0, 0)).x,
                                            mapToItem(container, Qt.point(0, 0)).y,
                                            width, height]
                    color: "blue"
                }

                property var layout1: RowLayout {
                    parent: container
                    anchors.fill: parent
                    spacing: 0
                    LayoutItemProxy { target: redRectanlge }
                    LayoutItemProxy { target: blueRectanlge }
                }

                property var layout2: ColumnLayout {
                    parent: container
                    anchors.fill: parent
                    spacing: 0
                    LayoutItemProxy { target: redRectanlge }
                    LayoutItemProxy { target: blueRectanlge }
                }

                property var layout3: ColumnLayout {
                    parent: container
                    anchors.fill: parent
                    spacing: 0
                    LayoutItemProxy { target: redRectanlge }
                    LayoutItemProxy { target: blueRectanlge }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "green"
                    }
                }
            }
        }

        function test_Proxy_native_item()
        {
            var item = createTemporaryObject(layout_proxy_Component_Three, container);

            item.layout1.visible = true
            item.layout2.visible = false
            item.layout3.visible = false

            tryCompare(item.rect1, "itemRect", [   0, 0, 100, 200])
            tryCompare(item.rect2, "itemRect", [ 100, 0, 100, 200])

            item.layout1.visible = false
            item.layout2.visible = true
            item.layout3.visible = false

            tryCompare(item.rect1, "itemRect", [ 0,   0, 200, 100])
            tryCompare(item.rect2, "itemRect", [ 0, 100, 200, 100])

            item.layout1.visible = false
            item.layout2.visible = false
            item.layout3.visible = true

            tryCompare(item.rect1, "itemRect", [ 0,  0, 200, 67]) //66.6 = 67
            tryCompare(item.rect2, "itemRect", [ 0, 67, 200, 66])
        }


        Component {
            id: layout_proxy_Component_overwrite
            Item {
                anchors.fill: container
                property var rect1: Rectangle {
                    id: redRectanlge
                    parent: container
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 180
                    Layout.preferredHeight: 180
                    Layout.margins: 0
                    property var itemRect: [mapToItem(container, Qt.point(0, 0)).x,
                                            mapToItem(container, Qt.point(0, 0)).y,
                                            width, height]
                    color: "red"
                }

                property var rect2: Rectangle {
                    id: blueRectanlge
                    parent: container
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                    Layout.margins: 0
                    property var itemRect: [mapToItem(container, Qt.point(0, 0)).x,
                                            mapToItem(container, Qt.point(0, 0)).y,
                                            width, height]
                    color: "blue"
                }

                property var layout1: RowLayout {
                    parent: container
                    anchors.fill: parent
                    spacing: 0
                    LayoutItemProxy { target: redRectanlge }
                    LayoutItemProxy { target: blueRectanlge }
                }

                property var layout2: ColumnLayout {
                    parent: container
                    anchors.fill: parent
                    spacing: 0
                    LayoutItemProxy { target: redRectanlge }
                    LayoutItemProxy { target: blueRectanlge }
                }
            }
        }

        function test_Proxy_overwrite_layout_properties()
        {
            var item = createTemporaryObject(layout_proxy_Component_overwrite, container);

            item.layout1.visible = true
            item.layout2.visible = false

            tryCompare(item.rect1, "itemRect", [   0, 0, 180, 200])
            tryCompare(item.rect2, "itemRect", [ 180, 0,  20, 200])

            item.layout1.visible = false
            item.layout2.visible = true

            tryCompare(item.rect1, "itemRect", [ 0,   0, 200, 180])
            tryCompare(item.rect2, "itemRect", [ 0, 180, 200,  20])

            //should overwrite the rectangles preferences
            item.layout1.children[0].Layout.preferredWidth = 100
            item.layout1.children[0].Layout.preferredHeight = 100
            item.layout1.children[1].Layout.preferredWidth = 100
            item.layout1.children[1].Layout.preferredHeight = 100

            item.layout1.visible = true
            item.layout2.visible = false

            tryCompare(item.rect1, "itemRect", [   0, 0, 100, 200])
            tryCompare(item.rect2, "itemRect", [ 100, 0, 100, 200])
        }

        Component {
            id: layout_proxy_Component_overwrite_declarative
            Item {
                anchors.fill: container
                property var rect1: Rectangle {
                    id: redRectanlge
                    parent: container
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumWidth: 1
                    Layout.minimumHeight: 1
                    Layout.maximumWidth: 3
                    Layout.maximumHeight: 3
                    Layout.preferredWidth: 2
                    Layout.preferredHeight: 2
                    Layout.margins: 1
                    Layout.leftMargin: 2
                    Layout.topMargin: 3
                    Layout.rightMargin: 4
                    Layout.bottomMargin: 5
                    Layout.alignment: Qt.AlignBottom
                    property var itemRect: [mapToItem(container, Qt.point(0, 0)).x,
                                            mapToItem(container, Qt.point(0, 0)).y,
                                            width, height]
                    color: "red"
                }


                property var layout1: RowLayout {
                    parent: container
                    anchors.fill: parent
                    spacing: 0
                    LayoutItemProxy { target: redRectanlge
                        Layout.fillWidth: false
                        Layout.fillHeight: false
                        Layout.minimumWidth: 100
                        Layout.minimumHeight: 100
                        Layout.maximumWidth: 300
                        Layout.maximumHeight: 300
                        Layout.preferredWidth: 200
                        Layout.preferredHeight: 200
                        Layout.margins: 100
                        Layout.leftMargin: 200
                        Layout.topMargin: 300
                        Layout.rightMargin: 400
                        Layout.bottomMargin: 500
                        Layout.alignment: Qt.AlignTop
                    }
                }

                property var layout2: RowLayout {
                    parent: container
                    anchors.fill: parent
                    spacing: 0
                    LayoutItemProxy { target: redRectanlge }
                }
            }
        }

        function test_Proxy_overwrite_layout_properties_declarative()
        {
            var item = createTemporaryObject(layout_proxy_Component_overwrite_declarative, container);

            compare(item.layout2.children[0].Layout.fillWidth, item.rect1.Layout.fillWidth)
            compare(item.layout2.children[0].Layout.fillHeight, item.rect1.Layout.fillHeight)
            compare(item.layout2.children[0].Layout.minimumWidth, item.rect1.Layout.minimumWidth)
            compare(item.layout2.children[0].Layout.minimumHeight, item.rect1.Layout.minimumHeight)
            compare(item.layout2.children[0].Layout.maximumWidth, item.rect1.Layout.maximumWidth)
            compare(item.layout2.children[0].Layout.maximumHeight, item.rect1.Layout.maximumHeight)
            compare(item.layout2.children[0].Layout.preferredWidth, item.rect1.Layout.preferredWidth)
            compare(item.layout2.children[0].Layout.preferredHeight, item.rect1.Layout.preferredHeight)
            compare(item.layout2.children[0].Layout.margins, item.rect1.Layout.margins)
            compare(item.layout2.children[0].Layout.leftMargin, item.rect1.Layout.leftMargin)
            compare(item.layout2.children[0].Layout.topMargin, item.rect1.Layout.topMargin)
            compare(item.layout2.children[0].Layout.rightMargin, item.rect1.Layout.rightMargin)
            compare(item.layout2.children[0].Layout.bottomMargin, item.rect1.Layout.bottomMargin)
            compare(item.layout2.children[0].Layout.alignment, item.rect1.Layout.alignment)

            verify(item.layout1.children[0].Layout.fillWidth != item.rect1.Layout.fillWidth)
            verify(item.layout1.children[0].Layout.fillHeight != item.rect1.Layout.fillHeight)
            verify(item.layout1.children[0].Layout.minimumWidth != item.rect1.Layout.minimumWidth)
            verify(item.layout1.children[0].Layout.minimumHeight != item.rect1.Layout.minimumHeight)
            verify(item.layout1.children[0].Layout.maximumWidth != item.rect1.Layout.maximumWidth)
            verify(item.layout1.children[0].Layout.maximumHeight != item.rect1.Layout.maximumHeight)
            verify(item.layout1.children[0].Layout.preferredWidth != item.rect1.Layout.preferredWidth)
            verify(item.layout1.children[0].Layout.preferredHeight != item.rect1.Layout.preferredHeight)
            verify(item.layout1.children[0].Layout.margins != item.rect1.Layout.margins)
            verify(item.layout1.children[0].Layout.leftMargin != item.rect1.Layout.leftMargin)
            verify(item.layout1.children[0].Layout.topMargin != item.rect1.Layout.topMargin)
            verify(item.layout1.children[0].Layout.rightMargin != item.rect1.Layout.rightMargin)
            verify(item.layout1.children[0].Layout.bottomMargin != item.rect1.Layout.bottomMargin)
            verify(item.layout1.children[0].Layout.alignment != item.rect1.alignment)

            compare(item.layout1.children[0].Layout.fillWidth, false)
            compare(item.layout1.children[0].Layout.fillHeight, false)
            compare(item.layout1.children[0].Layout.minimumWidth, item.rect1.Layout.minimumWidth * 100)
            compare(item.layout1.children[0].Layout.minimumHeight, item.rect1.Layout.minimumHeight * 100)
            compare(item.layout1.children[0].Layout.maximumWidth, item.rect1.Layout.maximumWidth * 100)
            compare(item.layout1.children[0].Layout.maximumHeight, item.rect1.Layout.maximumHeight * 100)
            compare(item.layout1.children[0].Layout.preferredWidth, item.rect1.Layout.preferredWidth * 100)
            compare(item.layout1.children[0].Layout.preferredHeight, item.rect1.Layout.preferredHeight * 100)
            compare(item.layout1.children[0].Layout.margins, item.rect1.Layout.margins * 100)
            compare(item.layout1.children[0].Layout.leftMargin, item.rect1.Layout.leftMargin * 100)
            compare(item.layout1.children[0].Layout.topMargin, item.rect1.Layout.topMargin * 100)
            compare(item.layout1.children[0].Layout.rightMargin, item.rect1.Layout.rightMargin * 100)
            compare(item.layout1.children[0].Layout.bottomMargin, item.rect1.Layout.bottomMargin * 100)
            compare(item.layout1.children[0].Layout.alignment, Qt.AlignTop)
        }

        Component {
            id: layout_proxy_Component_nesting
            Item {
                anchors.fill: container

                property var rect1: Rectangle {
                    id: redRectanlge
                    parent: container
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: 0
                    property var itemRect: [parent ? parent.mapToItem(container, Qt.point(x, y)).x : 0,
                                            parent ? parent.mapToItem(container, Qt.point(x, y)).y : 0,
                                            width, height]
                    color: "red"
                }


                property var rect2: Rectangle {
                    id: blueRectanlge
                    parent: container
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: 0
                    property var itemRect: [parent ? parent.mapToItem(container, Qt.point(x, y)).x : 0,
                                            parent ? parent.mapToItem(container, Qt.point(x, y)).y : 0,
                                            width, height]
                    color: "blue"
                }

                property var rect3: Rectangle {
                    id: greenRectanlge
                    parent: container
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: 0
                    property var itemRect: [parent ? parent.mapToItem(container, Qt.point(x, y)).x : 0,
                                            parent ? parent.mapToItem(container, Qt.point(x, y)).y : 0,
                                            width, height]
                    color: "green"
                }

                property var layout1: RowLayout {
                    parent: container
                    anchors.fill: parent
                    spacing: 0
                    LayoutItemProxy { target: redRectanlge }
                    ColumnLayout {
                        spacing: 0
                        LayoutItemProxy { target: blueRectanlge }
                        LayoutItemProxy { target: greenRectanlge }
                    }
                }

                property var layout2: ColumnLayout {
                    parent: container
                    anchors.fill: parent
                    spacing: 0
                    LayoutItemProxy { target: redRectanlge }
                    RowLayout {
                        spacing: 0
                        LayoutItemProxy { target: blueRectanlge }
                        LayoutItemProxy { target: greenRectanlge }
                    }
                }
            }
        }

        function test_Proxy_nesting()
        {
            var item = createTemporaryObject(layout_proxy_Component_nesting, container);

            item.layout2.visible = false
            item.layout1.visible = true

            tryCompare(item.rect1, "itemRect", [   0,   0, 100, 200])
            tryCompare(item.rect2, "itemRect", [ 100,   0, 100, 100])
            tryCompare(item.rect3, "itemRect", [ 100, 100, 100, 100])

            item.layout1.visible = false
            item.layout2.visible = true

            tryCompare(item.rect1, "itemRect", [   0,   0, 200, 100])
            tryCompare(item.rect2, "itemRect", [   0, 100, 100, 100])
            tryCompare(item.rect3, "itemRect", [ 100, 100, 100, 100])

            item.layout1.visible = true
            item.layout2.visible = false

            tryCompare(item.rect1, "itemRect", [   0,   0, 100, 200])
            tryCompare(item.rect2, "itemRect", [ 100,   0, 100, 100])
            tryCompare(item.rect3, "itemRect", [ 100, 100, 100, 100])
        }



        Component {
            id: layout_proxy_Component_nesting_item
            Item {
                anchors.fill: container

                property var rect1: Rectangle {
                    id: redRectanlge
                    parent: container
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: 0
                    property var itemRect: [parent ? parent.mapToItem(container, Qt.point(x, y)).x : 0,
                                            parent ? parent.mapToItem(container, Qt.point(x, y)).y : 0,
                                            width, height]
                    color: "red"
                }

                property var rect2: Rectangle {
                    id: blueRectanlge
                    parent: container
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: 0
                    property var itemRect: [parent ? parent.mapToItem(container, Qt.point(x, y)).x : 0,
                                            parent ? parent.mapToItem(container, Qt.point(x, y)).y : 0,
                                            width, height]
                    color: "blue"
                }

                property var rect3: Rectangle {
                    id: greenRectanlge
                    parent: container
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: 0
                    property var itemRect: [parent ? parent.mapToItem(container, Qt.point(x, y)).x : 0,
                                            parent ? parent.mapToItem(container, Qt.point(x, y)).y : 0,
                                            width, height]
                    color: "green"
                }

                property var rect4: Rectangle {
                    id: yellowRectangle
                    parent: container
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: 0
                    property var itemRect: [parent ? parent.mapToItem(container, Qt.point(x, y)).x : 0,
                                            parent ? parent.mapToItem(container, Qt.point(x, y)).y : 0,
                                            width, height]
                    color: "yellow"
                }

                property var rect5: Rectangle {
                    id: brownRectangle
                    parent: container
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: 0
                    property var itemRect: [parent ? parent.mapToItem(container, Qt.point(x, y)).x : 0,
                                            parent ? parent.mapToItem(container, Qt.point(x, y)).y : 0,
                                            width, height]
                    color: "brown"
                }

                property var layout1: RowLayout {
                    parent: container
                    anchors.fill: parent
                    spacing: 0
                    LayoutItemProxy { target: redRectanlge }
                    ColumnLayout {
                        spacing: 0
                        LayoutItemProxy { target: blueRectanlge }
                        LayoutItemProxy { target: greenRectanlge }
                        Item {
                            implicitWidth: rll.implicitWidth
                            implicitHeight: rll.implicitHeight
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            RowLayout {
                                id: rll
                                anchors.fill: parent
                                spacing: 0
                                LayoutItemProxy { target: yellowRectangle }
                                LayoutItemProxy { target: brownRectangle }
                            }
                        }
                    }
                }

                property var layout2: ColumnLayout {
                    parent: container
                    anchors.fill: parent
                    spacing: 0
                    LayoutItemProxy { target: redRectanlge }
                    RowLayout {
                        spacing: 0
                        LayoutItemProxy { target: blueRectanlge }
                        LayoutItemProxy { target: greenRectanlge }
                        Item {
                            implicitWidth: cll.implicitWidth
                            implicitHeight: cll.implicitHeight
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            ColumnLayout {
                                id: cll
                                anchors.fill: parent
                                spacing: 0
                                LayoutItemProxy { target: yellowRectangle }
                                LayoutItemProxy { target: brownRectangle }
                            }
                        }
                    }
                }
            }
        }

        function test_Proxy_nesting_item()
        {
            var item = createTemporaryObject(layout_proxy_Component_nesting_item, container);

            item.layout1.visible = true
            item.layout2.visible = false

            tryCompare(item.rect1, "itemRect", [   0,   0, 100, 200])
            tryCompare(item.rect2, "itemRect", [ 100,   0, 100,  67])
            tryCompare(item.rect3, "itemRect", [ 100,  67, 100,  66])
            tryCompare(item.rect4, "itemRect", [ 100, 133,  50,  67])
            tryCompare(item.rect5, "itemRect", [ 150, 133,  50,  67])

            item.layout2.visible = true
            item.layout1.visible = false

            tryCompare(item.rect1, "itemRect", [   0,   0, 200, 100])
            tryCompare(item.rect2, "itemRect", [   0, 100,  67, 100])
            tryCompare(item.rect3, "itemRect", [  67, 100,  66, 100])
            tryCompare(item.rect4, "itemRect", [ 133, 100,  67,  50])
            tryCompare(item.rect5, "itemRect", [ 133, 150,  67,  50])
        }
    }
}
