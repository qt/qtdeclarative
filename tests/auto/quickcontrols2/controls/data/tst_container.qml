// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtTest
import QtQuick.Controls
import QtQuick.Templates as T

TestCase {
    id: testCase
    width: 400
    height: 400
    visible: true
    when: windowShown
    name: "Container"

    Component {
        id: container
        Container { }
    }

    Component {
        id: rectangle
        Rectangle { }
    }

    function test_defaults() {
        failOnWarning(/.?/)

        let control = createTemporaryObject(container, testCase)
        verify(control)
        compare(control.count, 0)
    }

    function test_implicitSize() {
        var control = createTemporaryObject(container, testCase)
        verify(control)

        compare(control.implicitWidth, 0)
        compare(control.implicitHeight, 0)

        control.contentItem = rectangle.createObject(control, {implicitWidth: 10, implicitHeight: 20})
        compare(control.implicitWidth, 10)
        compare(control.implicitHeight, 20)

        control.background = rectangle.createObject(control, {implicitWidth: 20, implicitHeight: 30})
        compare(control.implicitWidth, 20)
        compare(control.implicitHeight, 30)

        control.padding = 100
        compare(control.implicitWidth, 210)
        compare(control.implicitHeight, 220)
    }

    function test_currentIndex() {
        var control1 = createTemporaryObject(container, testCase)
        verify(control1)

        var control2 = createTemporaryObject(container, testCase)
        verify(control2)

        compare(control1.currentIndex, -1)
        compare(control2.currentIndex, -1)

        for (var i = 0; i < 3; ++i) {
            control1.addItem(rectangle.createObject(control1))
            control2.addItem(rectangle.createObject(control2))
        }

        compare(control1.count, 3)
        compare(control2.count, 3)
        compare(control1.currentIndex, 0)
        compare(control2.currentIndex, 0)

        control1.currentIndex = Qt.binding(function() { return control2.currentIndex })
        control2.currentIndex = Qt.binding(function() { return control1.currentIndex })

        control1.setCurrentIndex(1)
        compare(control1.currentIndex, 1)
        compare(control2.currentIndex, 1)

        control1.incrementCurrentIndex()
        compare(control1.currentIndex, 2)
        compare(control2.currentIndex, 2)

        control2.decrementCurrentIndex()
        compare(control1.currentIndex, 1)
        compare(control2.currentIndex, 1)
    }

    Component {
        id: repeaterContainer1
        Container {
            id: container
            Item { objectName: "0" }
            Item { objectName: "1" }
            Item { objectName: "2" }
            Item { objectName: "3" }
            contentItem: Row {
                Repeater {
                    model: container.contentModel
                }
            }
        }
    }

    Component {
        id: repeaterContainer2
        Container {
            id: container
            contentItem: Item {
                Repeater {
                    model: container.contentModel
                }
                Rectangle { objectName: "extra" }
            }
            Rectangle { objectName: "0" }
            Rectangle { objectName: "1" }
            Rectangle { objectName: "2" }
            Rectangle { objectName: "3" }
        }
    }

    function test_repeater_data() {
        return [
            { tag: "1", component: repeaterContainer1 },
            { tag: "2", component: repeaterContainer2 }
        ]
    }

    // don't crash (QTBUG-61310)
    function test_repeater(data) {
        var control = createTemporaryObject(data.component, testCase)
        verify(control)

        compare(control.itemAt(0).objectName, "0")
        compare(control.itemAt(1).objectName, "1")
        compare(control.itemAt(2).objectName, "2")
        compare(control.itemAt(3).objectName, "3")
    }

    function test_removeTakeItem() {
        var control = createTemporaryObject(container, testCase)
        verify(control)

        var item1 = rectangle.createObject(control)
        var item2 = rectangle.createObject(control)
        var item3 = rectangle.createObject(control)

        item1.Component.onDestruction.connect(function() { item1 = null })
        item2.Component.onDestruction.connect(function() { item2 = null })
        item3.Component.onDestruction.connect(function() { item3 = null })

        control.addItem(item1)
        control.addItem(item2)
        control.addItem(item3)
        compare(control.count, 3)

        // takeItem(int) does not destroy
        compare(control.takeItem(1), item2)
        compare(control.count, 2)
        wait(1)
        verify(item2)

        // removeItem(Item) destroys
        control.removeItem(item1)
        compare(control.count, 1)
        wait(1)
        verify(!item1)

        // removeItem(null) must not call removeItem(0)
        control.removeItem(null)
        compare(control.count, 1)
        wait(1)
        verify(item3)
    }

    Component {
        id: contentItemDeletionOrder1

        Item {
            objectName: "parentItem"

            Item {
                id: item
                objectName: "contentItem"
            }
            Container {
                objectName: "control"
                contentItem: item
            }
        }
    }

    Component {
        id: contentItemDeletionOrder2

        Item {
            objectName: "parentItem"

            Container {
                objectName: "control"
                contentItem: item
            }
            Item {
                id: item
                objectName: "contentItem"
            }
        }
    }

    function test_contentItemDeletionOrder() {
        var control1 = createTemporaryObject(contentItemDeletionOrder1, testCase)
        verify(control1)
        var control2 = createTemporaryObject(contentItemDeletionOrder2, testCase)
        verify(control2)
    }

    Component {
        id: backgroundDeletionOrder1

        Item {
            objectName: "parentItem"

            Item {
                id: item
                objectName: "backgroundItem"
            }
            Container {
                objectName: "control"
                background: item
            }
        }
    }

    Component {
        id: backgroundDeletionOrder2

        Item {
            objectName: "parentItem"

            Container {
                objectName: "control"
                background: item
            }
            Item {
                id: item
                objectName: "backgroundItem"
            }
        }
    }

    function test_backgroundDeletionOrder() {
        var control1 = createTemporaryObject(backgroundDeletionOrder1, testCase)
        verify(control1)
        var control2 = createTemporaryObject(backgroundDeletionOrder2, testCase)
        verify(control2)
    }
}
