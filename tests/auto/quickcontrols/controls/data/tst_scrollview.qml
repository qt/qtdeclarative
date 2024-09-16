// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest
import QtQuick.Controls

TestCase {
    id: testCase
    width: 400
    height: 400
    visible: true
    when: windowShown
    name: "ScrollView"

    Component {
        id: signalSpyComponent
        SignalSpy { }
    }

    Component {
        id: scrollView
        ScrollView { }
    }

    Component {
        id: scrollBarComponent
        ScrollBar {}
    }

    Component {
        id: scrollableLabel
        ScrollView {
            Label {
                text: "ABC"
                font.pixelSize: 512
            }
        }
    }

    Component {
        id: scrollableLabels
        ScrollView {
            contentHeight: label1.implicitHeight + label2.implicitHeight + label3.implicitHeight
            Label {
                id: label1
                text: "First"
                font.pixelSize: 96
            }
            Label {
                id: label2
                text: "Second"
                font.pixelSize: 96
            }
            Label {
                id: label3
                text: "Third"
                font.pixelSize: 96
            }
        }
    }

    Component {
        id: flickableLabel
        ScrollView {
            Flickable {
                contentWidth: label.implicitWidth
                contentHeight: label.implicitHeight
                Label {
                    id: label
                    text: "ABC"
                    font.pixelSize: 512
                }
            }
        }
    }

    Component {
        id: emptyFlickable
        ScrollView {
            Flickable {
            }
        }
    }

    Component {
        id: labelComponent
        Label {
            text: "ABC"
            font.pixelSize: 512
        }
    }

    Component {
        id: scrollableListView
        ScrollView {
            ListView {
                model: 3
                delegate: Label {
                    text: modelData
                }
            }
        }
    }

    Component {
        id: scrollableFlickable
        ScrollView {
            Flickable {
                Item {
                    width: 100
                    height: 100
                }
            }
        }
    }

    Component {
        id: scrollableWithContentSize
        ScrollView {
            contentWidth: 1000
            contentHeight: 1000
            Flickable {
            }
        }
    }

    Component {
        id: scrollableAndFlicableWithContentSize
        ScrollView {
            contentWidth: 1000
            contentHeight: 1000
            Flickable {
                contentWidth: 200
                contentHeight: 200
            }
        }
    }

    Component {
        id: scrollableTextArea
        ScrollView {
            TextArea {
                text: "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Maecenas id dignissim ipsum. Nam molestie nisl turpis."
                wrapMode: TextArea.WordWrap
            }
        }
    }

    Component {
        id: scrollableTextAreaWithSibling
        ScrollView {
            Item {
            }
            TextArea {
            }
        }
    }

    function init() {
        failOnWarning(/.?/)
    }

    function test_defaults() {
        let control = createTemporaryObject(scrollView, testCase)
        verify(control)
    }

    function test_scrollBars() {
        let control = createTemporaryObject(scrollView, testCase, {width: 200, height: 200})
        verify(control)

        let vertical = control.ScrollBar.vertical
        verify(vertical)

        let horizontal = control.ScrollBar.horizontal
        verify(horizontal)

        control.contentHeight = 400
        verify(vertical.size > 0)
        compare(control.contentItem.visibleArea.heightRatio, vertical.size)

        control.contentWidth = 400
        verify(horizontal.size > 0)
        compare(control.contentItem.visibleArea.widthRatio, horizontal.size)

        vertical.increase()
        verify(vertical.position > 0)
        compare(control.contentItem.visibleArea.yPosition, vertical.position)

        horizontal.increase()
        verify(horizontal.position > 0)
        compare(control.contentItem.visibleArea.xPosition, horizontal.position)

        vertical.policy = ScrollBar.AlwaysOn
        horizontal.policy = ScrollBar.AlwaysOn

        verify(control.effectiveScrollBarWidth > 0)
        verify(control.effectiveScrollBarHeight > 0)

        vertical.policy = ScrollBar.AlwaysOff
        horizontal.policy = ScrollBar.AlwaysOff

        compare(control.effectiveScrollBarWidth, 0)
        compare(control.effectiveScrollBarHeight, 0)
    }

    function test_oneChild_data() {
        return [
            { tag: "label", component: scrollableLabel },
            { tag: "flickable", component: flickableLabel }
        ]
    }

    function test_oneChild(data) {
        let control = createTemporaryObject(data.component, testCase)
        verify(control)

        let flickable = control.contentItem
        verify(flickable.hasOwnProperty("contentX"))
        verify(flickable.hasOwnProperty("contentY"))

        let label = flickable.contentItem.children[0]
        compare(label.text, "ABC")

        compare(control.implicitWidth, label.implicitWidth)
        compare(control.implicitHeight, label.implicitHeight)

        compare(control.contentWidth, label.implicitWidth)
        compare(control.contentHeight, label.implicitHeight)

        compare(flickable.contentWidth, label.implicitWidth)
        compare(flickable.contentHeight, label.implicitHeight)

        control.contentWidth = 200
        compare(control.implicitWidth, 200)
        compare(control.contentWidth, 200)
        compare(flickable.contentWidth, 200)

        control.contentHeight = 100
        compare(control.implicitHeight, 100)
        compare(control.contentHeight, 100)
        compare(flickable.contentHeight, 100)
    }

    function test_multipleChildren() {
        let control = createTemporaryObject(scrollableLabels, testCase)
        verify(control)

        let flickable = control.contentItem
        verify(flickable.hasOwnProperty("contentX"))
        verify(flickable.hasOwnProperty("contentY"))

        compare(control.contentChildren, flickable.contentItem.children)

        let label1 = control.contentChildren[0]
        compare(label1.text, "First")

        let label2 = control.contentChildren[1]
        compare(label2.text, "Second")

        let label3 = control.contentChildren[2]
        compare(label3.text, "Third")

        let expectedContentHeight = label1.implicitHeight + label2.implicitHeight + label3.implicitHeight
        compare(control.contentHeight, expectedContentHeight)
        compare(flickable.contentHeight, expectedContentHeight)
    }

    function test_listView() {
        let control = createTemporaryObject(scrollableListView, testCase)
        verify(control)

        let listview = control.contentItem
        verify(listview.hasOwnProperty("contentX"))
        verify(listview.hasOwnProperty("contentY"))
        verify(listview.hasOwnProperty("model"))

        compare(control.contentWidth, listview.contentWidth)
        compare(control.contentHeight, listview.contentHeight)
    }

    function test_scrollableFlickable() {
        // Check that if the application adds a flickable as a child of a
        // scrollview, the scrollview doesn't try to calculate and change
        // the flickables contentWidth/Height based on the flickables
        // children, even if the flickable has an empty or negative content
        // size. Some flickables (e.g ListView) sets a negative
        // contentWidth on purpose, which should be respected.
        let scrollview = createTemporaryObject(scrollableFlickable, testCase)
        verify(scrollview)

        let flickable = scrollview.contentItem
        verify(flickable.hasOwnProperty("contentX"))
        verify(flickable.hasOwnProperty("contentY"))

        compare(flickable.contentWidth, -1)
        compare(flickable.contentHeight, -1)
        compare(scrollview.contentWidth, -1)
        compare(scrollview.contentHeight, -1)
    }

    function test_scrollableWithContentSize() {
        // Check that if the scrollview has contentWidth/Height set, but
        // not the flickable, then those values will be forwarded and used
        // by the flickable (rather than trying to calculate the content size
        // based on the flickables children).
        let scrollview = createTemporaryObject(scrollableWithContentSize, testCase)
        verify(scrollview)

        let flickable = scrollview.contentItem
        verify(flickable.hasOwnProperty("contentX"))
        verify(flickable.hasOwnProperty("contentY"))

        compare(flickable.contentWidth, 1000)
        compare(flickable.contentHeight, 1000)
        compare(scrollview.contentWidth, 1000)
        compare(scrollview.contentHeight, 1000)
    }

    function test_scrollableAndFlickableWithContentSize() {
        // Check that if both the scrollview and the flickable has
        // contentWidth/Height set (which is an inconsistency/fault
        // by the app), the content size of the scrollview wins.
        let scrollview = createTemporaryObject(scrollableAndFlicableWithContentSize, testCase)
        verify(scrollview)

        let flickable = scrollview.contentItem
        verify(flickable.hasOwnProperty("contentX"))
        verify(flickable.hasOwnProperty("contentY"))

        compare(flickable.contentWidth, 1000)
        compare(flickable.contentHeight, 1000)
        compare(scrollview.contentWidth, 1000)
        compare(scrollview.contentHeight, 1000)
    }

    function test_flickableWithExplicitContentSize() {
        let control = createTemporaryObject(emptyFlickable, testCase)
        verify(control)

        let flickable = control.contentItem
        verify(flickable.hasOwnProperty("contentX"))
        verify(flickable.hasOwnProperty("contentY"))

        let flickableContentSize = 1000;
        flickable.contentWidth = flickableContentSize;
        flickable.contentHeight = flickableContentSize;

        compare(flickable.contentWidth, flickableContentSize)
        compare(flickable.contentHeight, flickableContentSize)
        compare(control.implicitWidth, flickableContentSize)
        compare(control.implicitHeight, flickableContentSize)
        compare(control.contentWidth, flickableContentSize)
        compare(control.contentHeight, flickableContentSize)

        // Add a single child to the flickable. This should not
        // trick ScrollView into taking the implicit size of
        // the child as content size, since the flickable
        // already has an explicit content size.
        labelComponent.createObject(flickable);

        compare(flickable.contentWidth, flickableContentSize)
        compare(flickable.contentHeight, flickableContentSize)
        compare(control.implicitWidth, flickableContentSize)
        compare(control.implicitHeight, flickableContentSize)
        compare(control.contentWidth, flickableContentSize)
        compare(control.contentHeight, flickableContentSize)
    }

    function test_mouse() {
        let control = createTemporaryObject(scrollView, testCase, {width: 200, height: 200, contentHeight: 400})
        verify(control)

        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.contentItem.contentY, 0)

        for (let y = control.height / 2; y >= 0; --y) {
            mouseMove(control, control.width / 2, y, 10)
            compare(control.contentItem.contentY, 0)
        }

        mouseRelease(control, control.width / 2, 0, Qt.LeftButton)
        compare(control.contentItem.contentY, 0)
    }

    function test_hover() {
        let control = createTemporaryObject(scrollView, testCase, {width: 200, height: 200, contentHeight: 400})
        verify(control)

        let vertical = control.ScrollBar.vertical
        verify(vertical)
        vertical.hoverEnabled = true

        mouseMove(vertical, vertical.width / 2, vertical.height / 2)
        compare(vertical.visible, true)
        compare(vertical.hovered, true)
        compare(vertical.active, true)
        compare(vertical.interactive, true)
    }

    function test_wheel() {
        let control = createTemporaryObject(scrollView, testCase, {width: 200, height: 200, contentHeight: 400})
        verify(control)

        let vertical = control.ScrollBar.vertical
        verify(vertical)

        mouseWheel(control, control.width / 2, control.height / 2, 0, -120)
        compare(vertical.visible, true)
        compare(vertical.active, true)
        compare(vertical.interactive, true)
    }

    function test_touch() {
        let control = createTemporaryObject(scrollView, testCase, {width: 200, height: 200, contentHeight: 400})
        verify(control)

        let vertical = control.ScrollBar.vertical
        verify(vertical)

        let touch = touchEvent(control)
        touch.press(0, control, control.width / 2, control.height / 2).commit()
        compare(control.contentItem.contentY, 0)

        compare(vertical.active, false)
        compare(vertical.interactive, false)

        let maxContentY = 0
        for (let y = control.height / 2; y >= 0; --y) {
            touch.move(0, control, control.width / 2, y).commit()
            maxContentY = Math.max(maxContentY, control.contentItem.contentY)
        }
        verify(maxContentY > 0)

        compare(vertical.active, true)
        compare(vertical.interactive, false)

        touch.release(0, control, control.width / 2, 0).commit()
    }

    function test_keys() {
        let control = createTemporaryObject(scrollView, testCase, {width: 200, height: 200, contentWidth: 400, contentHeight: 400})
        verify(control)
        // If the viewport is smaller than the size of the ScrollView
        // (like windows style does due to its opaque scrollbars),
        // make the ScrollView taller in order to keep the *viewport* 200x200
        control.width += (control.width - control.availableWidth)
        control.height += (control.height - control.availableHeight)

        control.forceActiveFocus()
        verify(control.activeFocus)

        let vertical = control.ScrollBar.vertical
        verify(vertical)

        compare(vertical.position, 0.0)
        for (let i = 1; i <= 10; ++i) {
            keyClick(Qt.Key_Down)
            compare(vertical.position, Math.min(0.5, i * 0.1))
        }
        compare(vertical.position, 0.5)
        for (let i = 1; i <= 10; ++i) {
            keyClick(Qt.Key_Up)
            compare(vertical.position, Math.max(0.0, 0.5 - i * 0.1))
        }
        compare(vertical.position, 0.0)

        let horizontal = control.ScrollBar.horizontal
        verify(horizontal)

        compare(horizontal.position, 0.0)
        for (let i = 1; i <= 10; ++i) {
            keyClick(Qt.Key_Right)
            compare(horizontal.position, Math.min(0.5, i * 0.1))
        }
        compare(horizontal.position, 0.5)
        for (let i = 1; i <= 10; ++i) {
            keyClick(Qt.Key_Left)
            compare(horizontal.position, Math.max(0.0, 0.5 - i * 0.1))
        }
        compare(horizontal.position, 0.0)
    }

    function test_textArea() {
        // TODO: verify no binding loop warnings (QTBUG-62325)
        let control = createTemporaryObject(scrollableTextArea, testCase)
        verify(control)

        let flickable = control.contentItem
        verify(flickable && flickable.hasOwnProperty("contentX"))

        let textArea = flickable.contentItem.children[0]
        verify(textArea && textArea.hasOwnProperty("text"))

        compare(control.contentWidth, flickable.contentWidth)
        compare(control.contentHeight, flickable.contentHeight)
    }

    Component {
        id: scrollableTextAreaWithPadding

        ScrollView {
            TextArea {
                text: "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Maecenas id dignissim ipsum. Nam molestie nisl turpis."
                wrapMode: TextArea.WordWrap
                leftPadding: 1
                topPadding: 1
            }
        }
    }

    function test_textAreaWithPadding() {
        let control = createTemporaryObject(scrollableTextAreaWithPadding, testCase)
        verify(control)

        let flickable = control.contentItem
        verify(flickable)

        let textArea = flickable.contentItem.children[0]
        verify(textArea)

        compare(control.contentWidth, flickable.contentWidth)
        compare(control.contentHeight, flickable.contentHeight)
    }

    function test_textAreaWithSibling() {
        // Checks that it does not crash when the ScrollView is deleted
        let control = createTemporaryObject(scrollableTextAreaWithSibling, testCase)
        verify(control)
    }

    Component {
        id: zeroSizedContentItemComponent
        ScrollView {
            width: 100
            height: 100
            contentItem: Item {}
        }
    }

    function test_zeroSizedContentItem() {
        ignoreWarning(/ScrollView only supports Flickable types as its contentItem/)
        let control = createTemporaryObject(zeroSizedContentItemComponent, testCase)
        verify(control)

        let verticalScrollBar = control.ScrollBar.vertical
        verify(verticalScrollBar)
        // Scrolling a ScrollView with a zero-sized contentItem shouldn't crash.
        mouseDrag(verticalScrollBar, verticalScrollBar.width / 2, verticalScrollBar.height / 2, 0, 50)

        let horizontalScrollBar = control.ScrollBar.horizontal
        verify(verticalScrollBar)
        mouseDrag(horizontalScrollBar, horizontalScrollBar.width / 2, horizontalScrollBar.height / 2, 50, 0)
    }

    function test_customScrollBars() {
        let control = createTemporaryObject(scrollView, testCase)
        verify(control)
        control.ScrollBar.vertical.objectName = "oldVerticalScrollBar"
        control.ScrollBar.horizontal.objectName = "oldHorizontalScrollBar"

        let oldVerticalScrollBar = control.ScrollBar.vertical
        verify(oldVerticalScrollBar)
        compare(oldVerticalScrollBar.objectName, "oldVerticalScrollBar")

        let oldHorizontalScrollBar = control.ScrollBar.horizontal
        verify(oldHorizontalScrollBar)
        compare(oldHorizontalScrollBar.objectName, "oldHorizontalScrollBar")

        // Create the new scroll bars imperatively so that we can easily access the old ones.
        control.ScrollBar.vertical = scrollBarComponent.createObject(control, { objectName: "newVerticalScrollBar" })
        verify(control.ScrollBar.vertical)
        let newVerticalScrollBar = findChild(control, "newVerticalScrollBar")
        verify(newVerticalScrollBar)
        verify(newVerticalScrollBar.visible)
        verify(!oldVerticalScrollBar.visible)

        control.ScrollBar.horizontal = scrollBarComponent.createObject(control, { objectName: "newHorizontalScrollBar" })
        verify(control.ScrollBar.horizontal)
        let newHorizontalScrollBar = findChild(control, "newHorizontalScrollBar")
        verify(newHorizontalScrollBar)
        verify(newHorizontalScrollBar.visible)
        verify(!oldHorizontalScrollBar.visible)
    }

    Component {
        id: mouseAreaWheelComponent

        MouseArea {
            anchors.fill: parent

            property alias scrollView: scrollView
            property alias flickable: flickable

            ScrollView {
                id: scrollView
                anchors.fill: parent
                wheelEnabled: false

                Flickable {
                    id: flickable
                    contentHeight: 1000

                    Text {
                        text: "Test"
                        width: 500
                        height: 1000
                    }
                }
            }
        }
    }

    // If a ScrollView containing a Flickable sets wheelEnabled to false,
    // neither item should consume wheel events.
    function test_wheelEnabled() {
        let mouseArea = createTemporaryObject(mouseAreaWheelComponent, testCase)
        verify(mouseArea)

        let mouseWheelSpy = signalSpyComponent.createObject(mouseArea,
            { target: mouseArea, signalName: "wheel" })
        verify(mouseWheelSpy.valid)

        let scrollView = mouseArea.scrollView
        mouseWheel(scrollView, scrollView.width / 2, scrollView.height / 2, 0, 120)
        compare(mouseWheelSpy.count, 1)
        compare(mouseArea.flickable.contentY, 0)
    }

    Component {
        id: bindingToContentItemAndStandaloneFlickable

        Item {
            objectName: "container"
            width: 200
            height: 200

            property alias scrollView: scrollView

            ScrollView {
                id: scrollView
                anchors.fill: parent
                contentItem: listView

                property Item someBinding: contentItem
            }
            ListView {
                id: listView
                model: 10
                delegate: ItemDelegate {
                    text: modelData
                    width: listView.width
                }
            }
        }
    }

    // Tests that scroll bars show up for a ScrollView where
    // - its contentItem is declared as a standalone, separate item
    // - there is a binding to contentItem (which causes a default Flickable to be created)
    function test_bindingToContentItemAndStandaloneFlickable() {
        let root = createTemporaryObject(bindingToContentItemAndStandaloneFlickable, testCase)
        verify(root)

        let control = root.scrollView
        let verticalScrollBar = control.ScrollBar.vertical
        let horizontalScrollBar = control.ScrollBar.horizontal
        compare(verticalScrollBar.parent, control)
        compare(horizontalScrollBar.parent, control)
        verify(verticalScrollBar.visible)
        verify(horizontalScrollBar.visible)

        mouseWheel(control, control.width / 2, control.height / 2, 0, -120)
        verify(verticalScrollBar.active)
        verify(horizontalScrollBar.active)
    }

    Component {
        id: contentItemAssignedImperatively

        Item {
            objectName: "container"
            width: 100
            height: 100

            property alias scrollView: scrollView

            ListView {
                id: listView
                objectName: "customListView"
                model: 20
                delegate: Text {
                    text: modelData
                }
            }

            Component.onCompleted: scrollView.contentItem = listView

            ScrollView {
                id: scrollView
                anchors.fill: parent

                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            }
        }
    }

    // Tests that a ListView declared before the ScrollView (as the QObject destruction order
    // is relevant for the bug) and assigned imperatively to ScrollView does not cause:
    // - a crash on exit
    // - scroll bars that should be hidden to be visible
    function test_contentItemAssignedImperatively() {
        let root = createTemporaryObject(contentItemAssignedImperatively, testCase)
        verify(root)

        let control = root.scrollView
        let flickable = control.contentItem
        compare(flickable.parent, control)

        let horizontalScrollBar = control.ScrollBar.horizontal
        let verticalScrollBar = control.ScrollBar.vertical
        // The horizontal ScrollBar's policy is set to AlwaysOff, so it shouldn't ever be visible.
        verify(!horizontalScrollBar.visible)
        // The vertical ScrollBar should be visible...
        verify(verticalScrollBar.visible)

        // ... and it should become active when the ScrollView is scrolled.
        mouseWheel(control, control.width / 2, control.height / 2, 0, -120)
        verify(verticalScrollBar.active)

        // Shouldn't crash.
    }

    Component {
        id: scrollViewContentItemComp

        ScrollView {
            id: scrollView
            anchors.fill: parent
            Column {
                width: parent.width
                Repeater {
                    model: 20
                    Rectangle {
                        width: scrollView.width
                        height: 60
                        color: (index % 2 == 0) ? "red" : "green"
                    }
                }
            }
        }
    }

    function test_scrollViewContentItemSize() {
        let scrollview = createTemporaryObject(scrollViewContentItemComp, testCase)
        verify(scrollview)
        let contentItem = scrollview.contentItem
        waitForRendering(contentItem)
        compare(contentItem.contentWidth, 400)
        compare(contentItem.contentHeight, 1200)
        compare(scrollview.contentWidth, 400)
        compare(scrollview.contentHeight, 1200)
    }
}
