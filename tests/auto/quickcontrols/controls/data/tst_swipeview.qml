// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtTest
import QtQuick.Controls

TestCase {
    id: testCase
    width: 200
    height: 200
    visible: true
    when: windowShown
    name: "SwipeView"

    Component {
        id: swipeView
        SwipeView { }
    }

    Component {
        id: page
        Text { }
    }

    Component {
        id: signalSpy
        SignalSpy { }
    }

    function test_defaults() {
        failOnWarning(/.?/)

        let control = createTemporaryObject(swipeView, testCase)
        verify(control)
    }

    function test_current() {
        var control = createTemporaryObject(swipeView, testCase)

        var currentItemChangedSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "currentItemChanged"})
        verify(currentItemChangedSpy.valid)

        compare(control.count, 0)
        compare(control.currentIndex, -1)
        compare(control.currentItem, null)

        var item0 = page.createObject(control, {text: "0"})
        control.addItem(item0)
        compare(control.count, 1)
        compare(control.currentIndex, 0)
        compare(control.currentItem.text, "0")
        compare(currentItemChangedSpy.count, 1);
        compare(control.contentWidth, item0.implicitWidth)
        compare(control.contentHeight, item0.implicitHeight)

        var item1 = page.createObject(control, {text: "11"})
        control.addItem(item1)
        compare(control.count, 2)
        compare(control.currentIndex, 0)
        compare(control.currentItem.text, "0")
        compare(currentItemChangedSpy.count, 1);
        compare(control.contentWidth, item0.implicitWidth)
        compare(control.contentHeight, item0.implicitHeight)

        var item2 = page.createObject(control, {text: "222"})
        control.addItem(item2)
        compare(control.count, 3)
        compare(control.currentIndex, 0)
        compare(control.currentItem.text, "0")
        compare(currentItemChangedSpy.count, 1);
        compare(control.contentWidth, item0.implicitWidth)
        compare(control.contentHeight, item0.implicitHeight)

        control.currentIndex = 1
        compare(control.currentIndex, 1)
        compare(control.currentItem.text, "11")
        compare(currentItemChangedSpy.count, 2);
        compare(control.contentWidth, item1.implicitWidth)
        compare(control.contentHeight, item1.implicitHeight)

        control.currentIndex = 2
        compare(control.currentIndex, 2)
        compare(control.currentItem.text, "222")
        compare(currentItemChangedSpy.count, 3);
        compare(control.contentWidth, item2.implicitWidth)
        compare(control.contentHeight, item2.implicitHeight)

        control.decrementCurrentIndex()
        compare(control.currentIndex, 1)
        compare(control.currentItem.text, "11")
        compare(currentItemChangedSpy.count, 4);
        compare(control.contentWidth, item1.implicitWidth)
        compare(control.contentHeight, item1.implicitHeight)

        control.incrementCurrentIndex()
        compare(control.currentIndex, 2)
        compare(control.currentItem.text, "222")
        compare(currentItemChangedSpy.count, 5);
        compare(control.contentWidth, item2.implicitWidth)
        compare(control.contentHeight, item2.implicitHeight)
    }

    Component {
        id: initialCurrentSwipeView
        SwipeView {
            currentIndex: 1

            property alias item0: item0
            property alias item1: item1

            Item {
                id: item0
            }
            Item {
                id: item1
            }
        }
    }

    function test_initialCurrent() {
        var control = createTemporaryObject(initialCurrentSwipeView, testCase)

        compare(control.count, 2)
        compare(control.currentIndex, 1)
        compare(control.currentItem, control.item1)
    }

    function test_addRemove() {
        var control = createTemporaryObject(swipeView, testCase)

        function verifyCurrentIndexCountDiff() {
            verify(control.currentIndex < control.count)
        }
        control.currentIndexChanged.connect(verifyCurrentIndexCountDiff)
        control.countChanged.connect(verifyCurrentIndexCountDiff)

        var currentItemChangedSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "currentItemChanged"})
        verify(currentItemChangedSpy.valid)

        compare(control.count, 0)
        compare(control.currentIndex, -1)
        compare(control.currentItem, null)
        control.addItem(page.createObject(control, {text: "1"}))
        compare(control.count, 1)
        compare(control.currentIndex, 0)
        compare(currentItemChangedSpy.count, 1)
        compare(control.currentItem.text, "1")
        control.addItem(page.createObject(control, {text: "2"}))
        compare(control.count, 2)
        compare(control.currentIndex, 0)
        compare(currentItemChangedSpy.count, 1)
        compare(control.currentItem.text, "1")
        compare(control.itemAt(0).text, "1")
        compare(control.itemAt(1).text, "2")

        control.currentIndex = 1
        compare(currentItemChangedSpy.count, 2)

        control.insertItem(1, page.createObject(control, {text: "3"}))
        compare(control.count, 3)
        compare(control.currentIndex, 2)
        compare(control.currentItem.text, "2")
        compare(control.itemAt(0).text, "1")
        compare(control.itemAt(1).text, "3")
        compare(control.itemAt(2).text, "2")

        control.insertItem(0, page.createObject(control, {text: "4"}))
        compare(control.count, 4)
        compare(control.currentIndex, 3)
        compare(control.currentItem.text, "2")
        compare(control.itemAt(0).text, "4")
        compare(control.itemAt(1).text, "1")
        compare(control.itemAt(2).text, "3")
        compare(control.itemAt(3).text, "2")

        control.insertItem(control.count, page.createObject(control, {text: "5"}))
        compare(control.count, 5)
        compare(control.currentIndex, 3)
        compare(control.currentItem.text, "2")
        compare(control.itemAt(0).text, "4")
        compare(control.itemAt(1).text, "1")
        compare(control.itemAt(2).text, "3")
        compare(control.itemAt(3).text, "2")
        compare(control.itemAt(4).text, "5")

        control.removeItem(control.itemAt(control.count - 1))
        compare(control.count, 4)
        compare(control.currentIndex, 3)
        compare(control.currentItem.text, "2")
        compare(control.itemAt(0).text, "4")
        compare(control.itemAt(1).text, "1")
        compare(control.itemAt(2).text, "3")
        compare(control.itemAt(3).text, "2")

        control.removeItem(control.itemAt(0))
        compare(control.count, 3)
        compare(control.currentIndex, 2)
        compare(control.currentItem.text, "2")
        compare(control.itemAt(0).text, "1")
        compare(control.itemAt(1).text, "3")
        compare(control.itemAt(2).text, "2")

        control.removeItem(control.itemAt(1))
        compare(control.count, 2)
        compare(control.currentIndex, 1)
        compare(control.currentItem.text, "2")
        compare(control.itemAt(0).text, "1")
        compare(control.itemAt(1).text, "2")

        currentItemChangedSpy.clear()

        control.removeItem(control.itemAt(1))
        compare(control.count, 1)
        compare(control.currentIndex, 0)
        compare(currentItemChangedSpy.count, 1)
        compare(control.currentItem.text, "1")
        compare(control.itemAt(0).text, "1")

        control.removeItem(control.itemAt(0))
        compare(control.count, 0)
        compare(control.currentIndex, -1)
        compare(currentItemChangedSpy.count, 2)
    }

    Component {
        id: contentView
        SwipeView {
            QtObject { objectName: "object" }
            Item { objectName: "page1" }
            Timer { objectName: "timer" }
            Item { objectName: "page2" }
            Component { Item { } }
        }
    }

    function test_content() {
        var control = createTemporaryObject(contentView, testCase)

        function compareObjectNames(content, names) {
            if (content.length !== names.length)
                return false
            for (var i = 0; i < names.length; ++i) {
                if (content[i].objectName !== names[i])
                    return false
            }
            return true
        }

        verify(compareObjectNames(control.contentData, ["object", "page1", "timer", "page2", ""]))
        verify(compareObjectNames(control.contentChildren, ["page1", "page2"]))

        control.addItem(page.createObject(control, {objectName: "page3"}))
        verify(compareObjectNames(control.contentData, ["object", "page1", "timer", "page2", "", "page3"]))
        verify(compareObjectNames(control.contentChildren, ["page1", "page2", "page3"]))

        control.insertItem(0, page.createObject(control, {objectName: "page4"}))
        verify(compareObjectNames(control.contentData, ["object", "page1", "timer", "page2", "", "page3", "page4"]))
        verify(compareObjectNames(control.contentChildren, ["page4", "page1", "page2", "page3"]))

        control.moveItem(1, 2)
        verify(compareObjectNames(control.contentData, ["object", "page1", "timer", "page2", "", "page3", "page4"]))
        verify(compareObjectNames(control.contentChildren, ["page4", "page2", "page1", "page3"]))

        control.removeItem(control.itemAt(0))
        verify(compareObjectNames(control.contentData, ["object", "page1", "timer", "page2", "", "page3"]))
        verify(compareObjectNames(control.contentChildren, ["page2", "page1", "page3"]))
    }

    Component {
        id: repeated
        SwipeView {
            property alias repeater: repeater
            Repeater {
                id: repeater
                model: 5
                Item { property int idx: index }
            }
        }
    }

    function test_repeater() {
        var control = createTemporaryObject(repeated, testCase)
        verify(control)

        var model = control.contentModel
        verify(model)

        var repeater = control.repeater
        verify(repeater)

        compare(repeater.count, 5)
        compare(model.count, 5)

        for (var i = 0; i < 5; ++i) {
            var item1 = control.itemAt(i)
            verify(item1)
            compare(item1.idx, i)
            compare(model.get(i), item1)
            compare(repeater.itemAt(i), item1)
        }

        repeater.model = 3
        compare(repeater.count, 3)
        compare(model.count, 3)

        for (var j = 0; j < 3; ++j) {
            var item2 = control.itemAt(j)
            verify(item2)
            compare(item2.idx, j)
            compare(model.get(j), item2)
            compare(repeater.itemAt(j), item2)
        }
    }

    Component {
        id: ordered
        SwipeView {
            id: oview
            property alias repeater: repeater
            Text { text: "static_1" }
            Repeater {
                id: repeater
                model: 2
                Text { text: "repeated_" + (index + 2) }
            }
            Text { text: "static_4" }
            Component.onCompleted: {
                addItem(page.createObject(oview, {text: "dynamic_5"}))
                addItem(page.createObject(oview.contentItem, {text: "dynamic_6"}))
                insertItem(0, page.createObject(oview, {text: "dynamic_0"}))
            }
        }
    }

    function test_order() {
        var control = createTemporaryObject(ordered, testCase)
        verify(control)

        compare(control.count, 7)
        compare(control.itemAt(0).text, "dynamic_0")
        compare(control.itemAt(1).text, "static_1")
        compare(control.itemAt(2).text, "repeated_2")
        compare(control.itemAt(3).text, "repeated_3")
        compare(control.itemAt(4).text, "static_4")
        compare(control.itemAt(5).text, "dynamic_5")
        compare(control.itemAt(6).text, "dynamic_6")
    }

    function test_move_data() {
        return [
            {tag:"0->1 (0)", from: 0, to: 1, currentBefore: 0, currentAfter: 1},
            {tag:"0->1 (1)", from: 0, to: 1, currentBefore: 1, currentAfter: 0},
            {tag:"0->1 (2)", from: 0, to: 1, currentBefore: 2, currentAfter: 2},

            {tag:"0->2 (0)", from: 0, to: 2, currentBefore: 0, currentAfter: 2},
            {tag:"0->2 (1)", from: 0, to: 2, currentBefore: 1, currentAfter: 0},
            {tag:"0->2 (2)", from: 0, to: 2, currentBefore: 2, currentAfter: 1},

            {tag:"1->0 (0)", from: 1, to: 0, currentBefore: 0, currentAfter: 1},
            {tag:"1->0 (1)", from: 1, to: 0, currentBefore: 1, currentAfter: 0},
            {tag:"1->0 (2)", from: 1, to: 0, currentBefore: 2, currentAfter: 2},

            {tag:"1->2 (0)", from: 1, to: 2, currentBefore: 0, currentAfter: 0},
            {tag:"1->2 (1)", from: 1, to: 2, currentBefore: 1, currentAfter: 2},
            {tag:"1->2 (2)", from: 1, to: 2, currentBefore: 2, currentAfter: 1},

            {tag:"2->0 (0)", from: 2, to: 0, currentBefore: 0, currentAfter: 1},
            {tag:"2->0 (1)", from: 2, to: 0, currentBefore: 1, currentAfter: 2},
            {tag:"2->0 (2)", from: 2, to: 0, currentBefore: 2, currentAfter: 0},

            {tag:"2->1 (0)", from: 2, to: 1, currentBefore: 0, currentAfter: 0},
            {tag:"2->1 (1)", from: 2, to: 1, currentBefore: 1, currentAfter: 2},
            {tag:"2->1 (2)", from: 2, to: 1, currentBefore: 2, currentAfter: 1},

            {tag:"0->0", from: 0, to: 0, currentBefore: 0, currentAfter: 0},
            {tag:"-1->0", from: 0, to: 0, currentBefore: 1, currentAfter: 1},
            {tag:"0->-1", from: 0, to: 0, currentBefore: 2, currentAfter: 2},
            {tag:"1->10", from: 0, to: 0, currentBefore: 0, currentAfter: 0},
            {tag:"10->2", from: 0, to: 0, currentBefore: 1, currentAfter: 1},
            {tag:"10->-1", from: 0, to: 0, currentBefore: 2, currentAfter: 2}
        ]
    }

    Component {
        id: pageAttached

        Text {
            property int index: SwipeView.index
            property SwipeView view: SwipeView.view
            property bool isCurrentItem: SwipeView.isCurrentItem
            property bool isNextItem: SwipeView.isNextItem
            property bool isPreviousItem: SwipeView.isPreviousItem
        }
    }

    function test_move(data) {
        var control = createTemporaryObject(swipeView, testCase)

        compare(control.count, 0)
        var titles = ["1", "2", "3"]

        var i = 0;
        for (i = 0; i < titles.length; ++i) {
            var item = pageAttached.createObject(control, {text: titles[i]})
            control.addItem(item)
        }

        compare(control.count, titles.length)
        for (i = 0; i < control.count; ++i) {
            compare(control.itemAt(i).text, titles[i])
            compare(control.itemAt(i).SwipeView.index, i)
            compare(control.itemAt(i).SwipeView.isCurrentItem, i === 0)
            compare(control.itemAt(i).SwipeView.isNextItem, i === 1)
            compare(control.itemAt(i).SwipeView.isPreviousItem, false)
        }

        control.currentIndex = data.currentBefore
        for (i = 0; i < control.count; ++i) {
            compare(control.itemAt(i).SwipeView.isCurrentItem, i === data.currentBefore)
            compare(control.itemAt(i).SwipeView.isNextItem, i === data.currentBefore + 1)
            compare(control.itemAt(i).SwipeView.isPreviousItem, i === data.currentBefore - 1)
        }

        control.moveItem(data.from, data.to)

        compare(control.count, titles.length)
        compare(control.currentIndex, data.currentAfter)

        var title = titles[data.from]
        titles.splice(data.from, 1)
        titles.splice(data.to, 0, title)

        compare(control.count, titles.length)
        for (i = 0; i < control.count; ++i) {
            compare(control.itemAt(i).text, titles[i])
            compare(control.itemAt(i).SwipeView.index, i);
            compare(control.itemAt(i).SwipeView.isCurrentItem, i === data.currentAfter)
            compare(control.itemAt(i).SwipeView.isNextItem, i === data.currentAfter + 1)
            compare(control.itemAt(i).SwipeView.isPreviousItem, i === data.currentAfter - 1)
        }
    }

    Component {
        id: dynamicView
        SwipeView {
            id: dview
            Text { text: "static" }
            Component.onCompleted: {
                addItem(page.createObject(dview, {text: "added"}))
                insertItem(0, page.createObject(dview, {text: "inserted"}))
                page.createObject(dview, {text: "dynamic"})
            }
        }
    }

    function test_dynamic() {
        var control = createTemporaryObject(dynamicView, testCase)

        // insertItem(), addItem(), createObject() and static page {}
        compare(control.count, 4)
        compare(control.itemAt(0).text, "inserted")

        var tab = page.createObject(control, {text: "dying"})
        compare(control.count, 5)
        compare(control.itemAt(4).text, "dying")

        // TODO: fix crash in QQuickItemView
//        tab.destroy()
//        wait(0)
//        compare(control.count, 4)
    }

    function test_attachedParent() {
        var control = createTemporaryObject(swipeView, testCase);

        var page = createTemporaryObject(pageAttached, testCase);
        compare(page.view, null);
        compare(page.index, -1);
        compare(page.isCurrentItem, false);
        compare(page.isNextItem, false);
        compare(page.isPreviousItem, false);
        page.destroy();

        page = createTemporaryObject(pageAttached, null);
        compare(page.view, null);
        compare(page.index, -1);
        compare(page.isCurrentItem, false);
        compare(page.isNextItem, false);
        compare(page.isPreviousItem, false);

        control.insertItem(0, page);
        compare(control.count, 1);
        compare(page.parent, control.contentItem.contentItem);
        compare(page.view, control);
        compare(page.index, 0);
        compare(page.isCurrentItem, true);
        compare(page.isNextItem, false);
        compare(page.isPreviousItem, false);

        control.removeItem(control.itemAt(0));
        compare(control.count, 0);
        compare(page.parent, null);
        compare(page.view, null);
        compare(page.index, -1);
        compare(page.isCurrentItem, false);
        compare(page.isNextItem, false);
        compare(page.isPreviousItem, false);
    }

    function test_orientation() {
        var control = createTemporaryObject(swipeView, testCase, {width: 200, height: 200})
        verify(control)

        for (var i = 0; i < 3; ++i)
            control.addItem(page.createObject(control, {text: i}))

        compare(control.orientation, Qt.Horizontal)
        compare(control.horizontal, true)
        compare(control.vertical, false)

        for (i = 0; i < control.count; ++i) {
            control.currentIndex = i
            compare(control.itemAt(i).y, 0)
        }

        control.orientation = Qt.Vertical
        compare(control.orientation, Qt.Vertical)
        compare(control.horizontal, false)
        compare(control.vertical, true)

        for (i = 0; i < control.count; ++i) {
            control.currentIndex = i
            compare(control.itemAt(i).x, 0)
        }
    }

    Component {
        id: focusSwipeViewComponent

        SwipeView {
            id: swipeView
            anchors.fill: parent
            focus: true

            property int pressCount
            property int releaseCount
            property int rectanglePressCount
            property int rectangleReleaseCount

            Rectangle {
                focus: true

                Keys.onPressed: ++swipeView.rectanglePressCount
                Keys.onReleased: ++swipeView.rectangleReleaseCount
            }

            Keys.onPressed: ++pressCount
            Keys.onReleased: ++releaseCount
        }
    }

    function test_focus() {
        if (Qt.styleHints.tabFocusBehavior !== Qt.TabFocusAllControls)
            skip("This platform only allows tab focus for text controls")

        var control = createTemporaryObject(focusSwipeViewComponent, testCase)
        verify(control)
        compare(control.focus, true)
        compare(control.contentItem.focus, true)
        compare(control.itemAt(0).focus, true)
        compare(control.itemAt(0).activeFocus, true)

        keyPress(Qt.Key_A)
        compare(control.pressCount, 1)
        compare(control.releaseCount, 0)
        compare(control.rectanglePressCount, 1)
        compare(control.rectangleReleaseCount, 0)

        keyRelease(Qt.Key_A)
        compare(control.pressCount, 1)
        compare(control.releaseCount, 1)
        compare(control.rectanglePressCount, 1)
        compare(control.rectangleReleaseCount, 1)
    }

    // We have a particular customer who came up with this hack to make SwipeView wrap around at the end.
    // It's not advisible, and perhaps the test can be removed when we add a bool wrap property.
    Component {
        id: pathViewWorkaroundComponent

        SwipeView {
            id: swipeView
            anchors.left: parent.left
            width: 100
            height: 100
            clip: true
            Repeater {
                id: repeater
                objectName: "peter"
                delegate: Rectangle {
                    id: rect
                    color: "#ffff00"
                    border.color: "black"
                    width: 100
                    height: 100
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.horizontalCenter: parent.horizontalCenter
                        font.pixelSize: 24
                        text: model.index
                    }
                }
            }
            contentItem: PathView {
                id: pathview

                preferredHighlightBegin: 0.5
                preferredHighlightEnd: 0.5
                model: swipeView.contentModel

                path: Path {
                    id: path
                    startX: (swipeView.width / 2 * -1) * (swipeView.count - 1)
                    startY: swipeView.height / 2
                    PathLine {
                        relativeX: swipeView.width * swipeView.count
                        relativeY: 0
                    }
                }
            }
        }
    }

    function test_child_pathview() {
        const control = createTemporaryObject(pathViewWorkaroundComponent, testCase)
        verify(control)
        const repeater = control.children[0].children[0]
        const spy = signalSpy.createObject(repeater, {target: repeater, signalName: "itemAdded"})
        repeater.model = 1
        tryCompare(spy, "count", 1)
        const rect = repeater.itemAt(0)
        tryCompare(rect, "visible", true)
        if (Qt.platform.pluginName === "offscreen")
            skip("grabImage() is not functional on the offscreen platform (QTBUG-63185)")
        var image = grabImage(control)
        compare(image.pixel(3, 3), "#ffff00")
    }

    Component {
        id: translucentPages
        SwipeView {
            spacing: 10
            leftPadding: 10
            topPadding: 10
            rightPadding: 10
            bottomPadding: 10
            Text { text: "page 0" }
            Text { text: "page 1"; font.pointSize: 16 }
            Text { text: "page 2"; font.pointSize: 24 }
            Text { text: "page 3"; font.pointSize: 32 }
        }
    }

    function test_initialPositions() { // QTBUG-102487
        const control = createTemporaryObject(translucentPages, testCase, {width: 320, height: 200})
        verify(control)
        compare(control.contentItem.width, control.width - control.leftPadding - control.rightPadding)
        compare(control.spacing, 10)

        compare(control.orientation, Qt.Horizontal)
        for (let i = 0; i < control.count; ++i) {
            const page = control.itemAt(i)
            compare(page.x, i * (control.contentItem.width + control.spacing))
            compare(page.y, 0)
            compare(page.width, control.contentItem.width)
            compare(page.height, control.contentItem.height)
        }
        control.orientation = Qt.Vertical
        for (let i = 0; i < control.count; ++i) {
            const page = control.itemAt(i)
            compare(page.y, i * (control.contentItem.height + control.spacing))
            compare(page.x, 0)
            compare(page.width, control.contentItem.width)
            compare(page.height, control.contentItem.height)
        }

        // QTBUG-115468: add a page after startup and check that that works too.
        control.orientation = Qt.Horizontal
        let page4 = page.createObject(control, { text: "page 4", "font.pointSize": 40 })
        control.insertItem(control.count, page4)
        compare(page4.x, (control.count - 1) * 310)
        compare(page4.y, 0)
        compare(page4.width, control.contentItem.width)
        compare(page4.height, control.contentItem.height)
    }
}
