// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest
import QtQuick.Controls

TestCase {
    id: testCase
    width: 200
    height: 200
    visible: true
    when: windowShown
    name: "TabBar"

    Component {
        id: tabButton
        TabButton { }
    }

    Component {
        id: tabBar
        TabBar {
            topPadding: 0
            bottomPadding: 0
            leftPadding: 0
            rightPadding: 0
        }
    }

    Component {
        id: tabBarStaticTabs
        TabBar {
            TabButton {
                text: "0"
            }
            TabButton {
                text: "1"
            }
        }
    }

    Component {
        id: tabBarStaticTabsCurrent
        TabBar {
            currentIndex: 1
            TabButton {
                text: "0"
            }
            TabButton {
                text: "1"
            }
        }
    }

    Component {
        id: signalSpy
        SignalSpy { }
    }

    function init() {
        failOnWarning(/.?/)
    }

    function test_defaults() {
        let control = createTemporaryObject(tabBar, testCase)
        verify(control)
        compare(control.count, 0)
        compare(control.currentIndex, -1)
        compare(control.currentItem, null)
    }

    function test_current() {
        let control = createTemporaryObject(tabBar, testCase)

        compare(control.count, 0)
        compare(control.currentIndex, -1)
        compare(control.currentItem, null)

        control.addItem(tabButton.createObject(control, {text: "0"}))
        compare(control.count, 1)
        compare(control.currentIndex, 0)
        compare(control.currentItem.text, "0")
        compare(control.currentItem.checked, true)

        control.addItem(tabButton.createObject(control, {text: "1"}))
        compare(control.count, 2)
        compare(control.currentIndex, 0)
        compare(control.currentItem.text, "0")
        compare(control.currentItem.checked, true)

        control.addItem(tabButton.createObject(control, {text: "2"}))
        compare(control.count, 3)
        compare(control.currentIndex, 0)
        compare(control.currentItem.text, "0")
        compare(control.currentItem.checked, true)

        control.currentIndex = 1
        compare(control.currentIndex, 1)
        compare(control.currentItem.text, "1")
        compare(control.currentItem.checked, true)

        control.currentIndex = 2
        compare(control.currentIndex, 2)
        compare(control.currentItem.text, "2")
        compare(control.currentItem.checked, true)

        control.decrementCurrentIndex()
        compare(control.currentIndex, 1)
        compare(control.currentItem.text, "1")
        compare(control.currentItem.checked, true)

        control.incrementCurrentIndex()
        compare(control.currentIndex, 2)
        compare(control.currentItem.text, "2")
        compare(control.currentItem.checked, true)
    }

    function test_current_static() {
        let control = createTemporaryObject(tabBarStaticTabs, testCase)

        compare(control.count, 2)
        compare(control.currentIndex, 0)
        compare(control.currentItem.text, "0")
        compare(control.currentItem.checked, true)

        control = createTemporaryObject(tabBarStaticTabsCurrent, testCase)

        compare(control.count, 2)
        compare(control.currentIndex, 1)
        compare(control.currentItem.text, "1")
        compare(control.currentItem.checked, true)
    }

    function test_addRemove() {
        let control = createTemporaryObject(tabBar, testCase)

        function verifyCurrentIndexCountDiff() {
            verify(control.currentIndex < control.count)
        }
        control.currentIndexChanged.connect(verifyCurrentIndexCountDiff)
        control.countChanged.connect(verifyCurrentIndexCountDiff)

        let contentChildrenSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "contentChildrenChanged"})
        verify(contentChildrenSpy.valid)

        compare(control.count, 0)
        compare(control.currentIndex, -1)
        control.addItem(tabButton.createObject(control, {text: "1"}))
        compare(control.count, 1)
        compare(control.currentIndex, 0)
        compare(control.currentItem.text, "1")
        compare(contentChildrenSpy.count, 1)

        control.addItem(tabButton.createObject(control, {text: "2"}))
        compare(control.count, 2)
        compare(control.currentIndex, 0)
        compare(control.currentItem.text, "1")
        compare(control.itemAt(0).text, "1")
        compare(control.itemAt(1).text, "2")
        compare(contentChildrenSpy.count, 2)

        control.currentIndex = 1

        control.insertItem(1, tabButton.createObject(control, {text: "3"}))
        compare(control.count, 3)
        compare(control.currentIndex, 2)
        compare(control.currentItem.text, "2")
        compare(control.itemAt(0).text, "1")
        compare(control.itemAt(1).text, "3")
        compare(control.itemAt(2).text, "2")
        compare(contentChildrenSpy.count, 4) // append + insert->move

        control.insertItem(0, tabButton.createObject(control, {text: "4"}))
        compare(control.count, 4)
        compare(control.currentIndex, 3)
        compare(control.currentItem.text, "2")
        compare(control.itemAt(0).text, "4")
        compare(control.itemAt(1).text, "1")
        compare(control.itemAt(2).text, "3")
        compare(control.itemAt(3).text, "2")
        compare(contentChildrenSpy.count, 6) // append + insert->move

        control.insertItem(control.count, tabButton.createObject(control, {text: "5"}))
        compare(control.count, 5)
        compare(control.currentIndex, 3)
        compare(control.currentItem.text, "2")
        compare(control.itemAt(0).text, "4")
        compare(control.itemAt(1).text, "1")
        compare(control.itemAt(2).text, "3")
        compare(control.itemAt(3).text, "2")
        compare(control.itemAt(4).text, "5")
        compare(contentChildrenSpy.count, 7)

        control.removeItem(control.itemAt(control.count - 1))
        compare(control.count, 4)
        compare(control.currentIndex, 3)
        compare(control.currentItem.text, "2")
        compare(control.itemAt(0).text, "4")
        compare(control.itemAt(1).text, "1")
        compare(control.itemAt(2).text, "3")
        compare(control.itemAt(3).text, "2")
        compare(contentChildrenSpy.count, 8)

        control.removeItem(control.itemAt(0))
        compare(control.count, 3)
        compare(control.currentIndex, 2)
        compare(control.currentItem.text, "2")
        compare(control.itemAt(0).text, "1")
        compare(control.itemAt(1).text, "3")
        compare(control.itemAt(2).text, "2")
        compare(contentChildrenSpy.count, 9)

        control.removeItem(control.itemAt(1))
        compare(control.count, 2)
        compare(control.currentIndex, 1)
        compare(control.currentItem.text, "2")
        compare(control.itemAt(0).text, "1")
        compare(control.itemAt(1).text, "2")
        compare(contentChildrenSpy.count, 10)

        control.removeItem(control.itemAt(1))
        compare(control.count, 1)
        compare(control.currentIndex, 0)
        compare(control.currentItem.text, "1")
        compare(control.itemAt(0).text, "1")
        compare(contentChildrenSpy.count, 11)

        control.removeItem(control.itemAt(0))
        compare(control.count, 0)
        compare(control.currentIndex, -1)
        compare(contentChildrenSpy.count, 12)
    }

    function test_removeCurrent() {
        let control = createTemporaryObject(tabBar, testCase)

        control.addItem(tabButton.createObject(control, {text: "1"}))
        control.addItem(tabButton.createObject(control, {text: "2"}))
        control.addItem(tabButton.createObject(control, {text: "3"}))
        control.currentIndex = 1
        compare(control.count, 3)
        compare(control.currentIndex, 1)

        control.removeItem(control.itemAt(1))
        compare(control.count, 2)
        compare(control.currentIndex, 0)

        control.removeItem(control.itemAt(0))
        compare(control.count, 1)
        compare(control.currentIndex, 0)

        control.removeItem(control.itemAt(0))
        compare(control.count, 0)
        compare(control.currentIndex, -1)
    }

    Component {
        id: contentBar
        TabBar {
            QtObject { objectName: "object" }
            TabButton { objectName: "button1" }
            Timer { objectName: "timer" }
            TabButton { objectName: "button2" }
            Component { TabButton { } }
        }
    }

    function test_content() {
        let control = createTemporaryObject(contentBar, testCase)

        function compareObjectNames(content, names) {
            if (content.length !== names.length)
                return false
            for (let i = 0; i < names.length; ++i) {
                if (content[i].objectName !== names[i])
                    return false
            }
            return true
        }

        let contentChildrenSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "contentChildrenChanged"})
        verify(contentChildrenSpy.valid)

        verify(compareObjectNames(control.contentData, ["object", "button1", "timer", "button2", ""]))
        verify(compareObjectNames(control.contentChildren, ["button1", "button2"]))

        control.addItem(tabButton.createObject(control, {objectName: "button3"}))
        verify(compareObjectNames(control.contentData, ["object", "button1", "timer", "button2", "", "button3"]))
        verify(compareObjectNames(control.contentChildren, ["button1", "button2", "button3"]))
        compare(contentChildrenSpy.count, 1)

        control.insertItem(0, tabButton.createObject(control, {objectName: "button4"}))
        verify(compareObjectNames(control.contentData, ["object", "button1", "timer", "button2", "", "button3", "button4"]))
        verify(compareObjectNames(control.contentChildren, ["button4", "button1", "button2", "button3"]))
        compare(contentChildrenSpy.count, 3) // append + insert->move

        control.moveItem(1, 2)
        verify(compareObjectNames(control.contentData, ["object", "button1", "timer", "button2", "", "button3", "button4"]))
        verify(compareObjectNames(control.contentChildren, ["button4", "button2", "button1", "button3"]))
        compare(contentChildrenSpy.count, 4)

        control.removeItem(control.itemAt(0))
        verify(compareObjectNames(control.contentData, ["object", "button1", "timer", "button2", "", "button3"]))
        verify(compareObjectNames(control.contentChildren, ["button2", "button1", "button3"]))
        compare(contentChildrenSpy.count, 5)
    }

    Component {
        id: repeated
        TabBar {
            property alias repeater: repeater
            Repeater {
                id: repeater
                model: 5
                TabButton { property int idx: index }
            }
        }
    }

    function test_repeater() {
        let control = createTemporaryObject(repeated, testCase)
        verify(control)

        let model = control.contentModel
        verify(model)

        let repeater = control.repeater
        verify(repeater)

        compare(repeater.count, 5)
        compare(model.count, 5)

        for (let i = 0; i < 5; ++i) {
            let item1 = control.itemAt(i)
            verify(item1)
            compare(item1.idx, i)
            compare(model.get(i), item1)
            compare(repeater.itemAt(i), item1)
        }

        repeater.model = 3
        compare(repeater.count, 3)
        compare(model.count, 3)

        for (let j = 0; j < 3; ++j) {
            let item2 = control.itemAt(j)
            verify(item2)
            compare(item2.idx, j)
            compare(model.get(j), item2)
            compare(repeater.itemAt(j), item2)
        }
    }

    Component {
        id: ordered
        TabBar {
            id: obar
            property alias repeater: repeater
            TabButton { text: "static_1" }
            Repeater {
                id: repeater
                model: 2
                TabButton { text: "repeated_" + (index + 2) }
            }
            TabButton { text: "static_4" }
            Component.onCompleted: {
                addItem(tabButton.createObject(obar, {text: "dynamic_5"}))
                addItem(tabButton.createObject(obar.contentItem, {text: "dynamic_6"}))
                insertItem(0, tabButton.createObject(obar, {text: "dynamic_0"}))
            }
        }
    }

    function test_order() {
        let control = createTemporaryObject(ordered, testCase)
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

    function test_move(data) {
        let control = createTemporaryObject(tabBar, testCase)

        compare(control.count, 0)
        let titles = ["1", "2", "3"]

        let i = 0;
        for (i = 0; i < titles.length; ++i)
            control.addItem(tabButton.createObject(control, {text: titles[i]}))

        compare(control.count, titles.length)
        for (i = 0; i < control.count; ++i)
            compare(control.itemAt(i).text, titles[i])

        control.currentIndex = data.currentBefore
        control.moveItem(data.from, data.to)

        compare(control.count, titles.length)
        compare(control.currentIndex, data.currentAfter)

        let title = titles[data.from]
        titles.splice(data.from, 1)
        titles.splice(data.to, 0, title)

        compare(control.count, titles.length)
        for (i = 0; i < control.count; ++i)
            compare(control.itemAt(i).text, titles[i])
    }

    Component {
        id: dynamicBar
        TabBar {
            id: dbar
            TabButton { text: "static" }
            Component.onCompleted: {
                addItem(tabButton.createObject(dbar, {text: "added"}))
                insertItem(0, tabButton.createObject(dbar, {text: "inserted"}))
                tabButton.createObject(dbar, {text: "dynamic"})
            }
        }
    }

    function test_dynamic() {
        let control = createTemporaryObject(dynamicBar, testCase)

        // insertItem(), addItem(), createObject() and static TabButton {}
        compare(control.count, 4)
        compare(control.itemAt(0).text, "inserted")

        let tab = tabButton.createObject(control, {text: "dying"})
        compare(control.count, 5)
        compare(control.itemAt(4).text, "dying")

        // TODO: fix crash in QQuickItemView
//        tab.destroy()
//        wait(0)
//        compare(control.count, 4)
    }

    function test_layout_data() {
        return [
            { tag: "spacing:0", spacing: 0 },
            { tag: "spacing:1", spacing: 1 },
            { tag: "spacing:10", spacing: 10 },
        ]
    }

    function test_layout(data) {
        let control = createTemporaryObject(tabBar, testCase, {spacing: data.spacing, width: 200})

        // remove the background so that it won't affect the implicit size of the tabbar,
        // so the implicit sizes tested below are entirely based on the content size
        control.background = null

        let tab1 = tabButton.createObject(control, {text: "First"})
        control.addItem(tab1)
        tryCompare(tab1, "width", control.width)
        compare(tab1.height, control.height)
        compare(control.implicitContentWidth, tab1.implicitWidth)
        compare(control.implicitContentHeight, tab1.implicitHeight)
        compare(control.contentWidth, control.implicitContentWidth)
        compare(control.contentHeight, control.implicitContentHeight)
        compare(control.implicitWidth, control.contentWidth + control.leftPadding + control.rightPadding)
        compare(control.implicitHeight, control.contentHeight + control.topPadding + control.bottomPadding)

        let tab2 = tabButton.createObject(control, {implicitHeight: tab1.implicitHeight + 10, text: "Second"})
        control.addItem(tab2)
        tryCompare(tab1, "width", (control.width - data.spacing) / 2)
        compare(tab1.height, control.height)
        compare(tab2.width, (control.width - data.spacing) / 2)
        compare(tab2.height, control.height)
        compare(control.implicitContentWidth, tab1.implicitWidth + tab2.implicitWidth + data.spacing)
        compare(control.implicitContentHeight, tab2.implicitHeight)
        compare(control.contentWidth, control.implicitContentWidth)
        compare(control.contentHeight, control.implicitContentHeight)
        compare(control.implicitWidth, control.contentWidth + control.leftPadding + control.rightPadding)
        compare(control.implicitHeight, control.contentHeight + control.topPadding + control.bottomPadding)

        let tab3 = tabButton.createObject(control, {width: 50, height: tab1.implicitHeight - 10, text: "Third"})
        control.addItem(tab3)
        tryCompare(tab1, "width", (control.width - 2 * data.spacing - 50) / 2)
        compare(tab1.y, 0)
        compare(tab1.height, control.height)
        compare(tab2.y, 0)
        compare(tab2.width, (control.width - 2 * data.spacing - 50) / 2)
        compare(tab2.height, control.height)
        verify(tab3.y > 0)
        compare(tab3.y, (control.height - tab3.height) / 2)
        compare(tab3.width, 50)
        compare(tab3.height, tab1.implicitHeight - 10)
        compare(control.implicitContentWidth, tab1.implicitWidth + tab2.implicitWidth + tab3.width + 2 * data.spacing)
        compare(control.implicitContentHeight, tab2.implicitHeight)
        compare(control.contentWidth, control.implicitContentWidth)
        compare(control.contentHeight, control.implicitContentHeight)
        compare(control.implicitWidth, control.contentWidth + control.leftPadding + control.rightPadding)
        compare(control.implicitHeight, control.contentHeight + control.topPadding + control.bottomPadding)

        let expectedWidth = Math.max(tab3.implicitBackgroundWidth + tab3.leftInset + tab3.rightInset,
                                     tab3.implicitContentWidth + tab3.leftPadding + tab3.rightPadding)
        tab3.width = tab3.implicitWidth
        tab3.height = tab3.implicitHeight
        tryCompare(tab1, "width", (control.width - 2 * data.spacing - expectedWidth) / 2)
        compare(tab1.height, control.height)
        compare(tab2.width, (control.width - 2 * data.spacing - expectedWidth) / 2)
        compare(tab2.height, control.height)
        compare(tab3.width, expectedWidth)
        compare(tab3.height, tab3.implicitHeight)
        compare(control.implicitContentWidth, tab1.implicitWidth + tab2.implicitWidth + tab3.implicitWidth + 2 * data.spacing)
        compare(control.implicitContentHeight, tab2.implicitHeight)
        compare(control.contentWidth, control.implicitContentWidth)
        compare(control.contentHeight, control.implicitContentHeight)
        compare(control.implicitWidth, control.contentWidth + control.leftPadding + control.rightPadding)
        compare(control.implicitHeight, control.contentHeight + control.topPadding + control.bottomPadding)

        tab3.width = undefined
        tab3.height = undefined
        control.width = undefined

        control.contentWidth = 300
        control.contentHeight = 50
        expectedWidth = (control.contentWidth - 2 * data.spacing) / 3
        tryCompare(tab1, "width", expectedWidth)
        compare(tab2.width, expectedWidth)
        compare(tab3.width, expectedWidth)
        compare(tab1.height, control.contentHeight)
        compare(tab2.height, control.contentHeight)
        compare(tab3.height, control.contentHeight)
    }

    Component {
        id: wheelEnabledTabBar
        TabBar {
            wheelEnabled: true
            TabButton { text: "tab1" }
            TabButton { text: "tab2" }
        }
    }

    function test_wheelEnabled() {
        let control = createTemporaryObject(wheelEnabledTabBar, testCase, {width: 100, height: 100})
        verify(control)

        let deltas = [
            120, // common mouse wheel
            16 // high resolution mouse wheel
        ]

        for (let delta of deltas) {
            // increment
            for (let accumulated = 0; accumulated < 120; accumulated += delta) {
                // ensure index doesn't change until threshold is reached
                compare(control.currentIndex, 0)
                mouseWheel(control, control.width / 2, control.height / 2, -delta, -delta)
            }
            compare(control.currentIndex, 1)

            // reached bounds -> no change
            for (let accumulated = 0; accumulated < 120; accumulated += delta) {
                mouseWheel(control, control.width / 2, control.height / 2, -delta, -delta)
            }
            compare(control.currentIndex, 1)

            // decrement
            for (let accumulated = 0; accumulated < 120; accumulated += delta) {
                // ensure index doesn't change until threshold is reached
                compare(control.currentIndex, 1)
                mouseWheel(control, control.width / 2, control.height / 2, delta, delta)
            }
            compare(control.currentIndex, 0)

            // reached bounds -> no change
            for (let accumulated = 0; accumulated < 120; accumulated += delta) {
                mouseWheel(control, control.width / 2, control.height / 2, delta, delta)
            }
            compare(control.currentIndex, 0)
        }
    }

    Component {
        id: attachedButton
        TabButton {
            property int index: TabBar.index
            property TabBar tabBar: TabBar.tabBar
            property int position: TabBar.position
        }
    }

    function test_attached() {
        let control = createTemporaryObject(tabBar, testCase, {position: TabBar.Footer})

        // append
        let tab1 = createTemporaryObject(attachedButton, testCase)
        compare(tab1.index, -1)
        compare(tab1.tabBar, null)
        compare(tab1.position, TabBar.Header)

        control.addItem(tab1)
        compare(tab1.index, 0)
        compare(tab1.tabBar, control)
        compare(tab1.position, TabBar.Footer)

        // insert in the beginning
        let tab2 = createTemporaryObject(attachedButton, testCase)
        compare(tab2.index, -1)
        compare(tab2.tabBar, null)
        compare(tab2.position, TabBar.Header)

        control.insertItem(0, tab2)
        compare(tab2.index, 0)
        compare(tab2.tabBar, control)
        compare(tab2.position, TabBar.Footer)

        compare(tab1.index, 1)

        // insert in the middle
        let tab3 = createTemporaryObject(attachedButton, testCase)
        compare(tab3.index, -1)
        compare(tab3.tabBar, null)
        compare(tab3.position, TabBar.Header)

        control.insertItem(1, tab3)
        compare(tab3.index, 1)
        compare(tab3.tabBar, control)
        compare(tab3.position, TabBar.Footer)

        compare(tab2.index, 0)
        compare(tab1.index, 2)

        // insert in the end
        let tab4 = createTemporaryObject(attachedButton, testCase)
        compare(tab4.index, -1)
        compare(tab4.tabBar, null)
        compare(tab4.position, TabBar.Header)

        control.insertItem(-1, tab4)
        compare(tab4.index, 3)
        compare(tab4.tabBar, control)
        compare(tab4.position, TabBar.Footer)

        compare(tab2.index, 0)
        compare(tab3.index, 1)
        compare(tab1.index, 2)

        // move forwards
        control.moveItem(0, 1)
        compare(tab3.index, 0)
        compare(tab2.index, 1)
        compare(tab1.index, 2)
        compare(tab4.index, 3)

        control.moveItem(0, 2)
        compare(tab2.index, 0)
        compare(tab1.index, 1)
        compare(tab3.index, 2)
        compare(tab4.index, 3)

        control.moveItem(1, 3)
        compare(tab2.index, 0)
        compare(tab3.index, 1)
        compare(tab4.index, 2)
        compare(tab1.index, 3)

        // move backwards
        control.moveItem(3, 2)
        compare(tab2.index, 0)
        compare(tab3.index, 1)
        compare(tab1.index, 2)
        compare(tab4.index, 3)

        control.moveItem(3, 1)
        compare(tab2.index, 0)
        compare(tab4.index, 1)
        compare(tab3.index, 2)
        compare(tab1.index, 3)

        // remove from the beginning
        control.removeItem(control.itemAt(0))
        compare(tab2.index, -1)
        compare(tab2.tabBar, null)
        compare(tab2.position, TabBar.Header)

        compare(tab4.index, 0)
        compare(tab3.index, 1)
        compare(tab1.index, 2)

        // remove from the middle
        control.removeItem(control.itemAt(1))
        compare(tab3.index, -1)
        compare(tab3.tabBar, null)
        compare(tab3.position, TabBar.Header)

        compare(tab4.index, 0)
        compare(tab1.index, 1)
    }
}
