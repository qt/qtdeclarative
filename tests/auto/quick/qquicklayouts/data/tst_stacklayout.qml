/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
    }
}
