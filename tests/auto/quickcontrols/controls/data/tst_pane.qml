// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtTest
import QtQuick.Controls

TestCase {
    id: testCase
    width: 400
    height: 400
    visible: true
    when: windowShown
    name: "Pane"

    Component {
        id: pane
        Pane { }
    }

    Component {
        id: oneChildPane
        Pane {
            Item {
                implicitWidth: 100
                implicitHeight: 30
            }
        }
    }

    Component {
        id: twoChildrenPane
        Pane {
            Item {
                implicitWidth: 100
                implicitHeight: 30
            }
            Item {
                implicitWidth: 200
                implicitHeight: 60
            }
        }
    }

    Component {
        id: contentPane
        Pane {
            contentItem: Item {
                implicitWidth: 100
                implicitHeight: 30
            }
        }
    }

    Component {
        id: pressPane
        MouseArea {
            width: 200
            height: 200
            property int pressCount
            onPressed: ++pressCount
            Pane {
                anchors.fill: parent
            }
        }
    }

    function test_empty() {
        failOnWarning(/.?/)

        var control = createTemporaryObject(pane, testCase)
        verify(control)

        verify(control.contentItem)
        compare(control.contentWidth, 0)
        compare(control.contentHeight, 0)
        compare(control.implicitContentWidth, 0)
        compare(control.implicitContentHeight, 0)
    }

    function test_oneChild() {
        var control = createTemporaryObject(oneChildPane, testCase)
        verify(control)

        compare(control.contentWidth, 100)
        compare(control.contentHeight, 30)
        compare(control.implicitContentWidth, 100)
        compare(control.implicitContentHeight, 30)
        verify(control.implicitWidth > 100)
        verify(control.implicitHeight > 30)

        compare(control.contentChildren.length, 1)
        control.contentChildren[0].implicitWidth = 200
        control.contentChildren[0].implicitHeight = 40

        compare(control.contentWidth, 200)
        compare(control.contentHeight, 40)
        compare(control.implicitContentWidth, 200)
        compare(control.implicitContentHeight, 40)
        verify(control.implicitWidth > 200)
        verify(control.implicitHeight > 40)
    }

    function test_twoChildren() {
        var control = createTemporaryObject(twoChildrenPane, testCase)
        verify(control)

        compare(control.contentWidth, 0)
        compare(control.contentHeight, 0)
        compare(control.implicitContentWidth, 0)
        compare(control.implicitContentHeight, 0)
        verify(control.implicitWidth > 0)
        verify(control.implicitHeight > 0)
    }

    function test_contentItem() {
        var control = createTemporaryObject(contentPane, testCase)
        verify(control)

        compare(control.contentWidth, 100)
        compare(control.contentHeight, 30)
        compare(control.implicitContentWidth, 100)
        compare(control.implicitContentHeight, 30)
        verify(control.implicitWidth > 100)
        verify(control.implicitHeight > 30)
    }

    function test_implicitContentItem() {
        var control = createTemporaryObject(pane, testCase, {width: 100, height: 100})
        verify(control)

        compare(control.width, 100)
        compare(control.height, 100)
        compare(control.contentItem.width, control.availableWidth)
        compare(control.contentItem.height, control.availableHeight)
    }

    function test_press() {
        var control = createTemporaryObject(pressPane, testCase)
        verify(control)

        compare(control.pressCount, 0)
        mouseClick(control)
        compare(control.pressCount, 0)

        control.children[0].enabled = false
        mouseClick(control)
        compare(control.pressCount, 1)
    }
}
