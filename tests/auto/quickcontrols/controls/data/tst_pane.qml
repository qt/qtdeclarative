// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest
import QtQuick.Controls
import QtQuick.Layouts

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

    function init() {
        failOnWarning(/.?/)
    }

    function test_implicitContentItem() {
        let control = createTemporaryObject(pane, testCase, {width: 100, height: 100})
        verify(control)

        compare(control.width, 100)
        compare(control.height, 100)
        compare(control.contentItem.width, control.availableWidth)
        compare(control.contentItem.height, control.availableHeight)
    }

    function test_empty() {
        let control = createTemporaryObject(pane, testCase)
        verify(control)

        verify(control.contentItem)
        compare(control.contentWidth, 0)
        compare(control.contentHeight, 0)
        compare(control.implicitContentWidth, 0)
        compare(control.implicitContentHeight, 0)
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

    function test_oneChild() {
        let control = createTemporaryObject(oneChildPane, testCase)
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

    function test_twoChildren() {
        let control = createTemporaryObject(twoChildrenPane, testCase)
        verify(control)

        compare(control.contentWidth, 0)
        compare(control.contentHeight, 0)
        compare(control.implicitContentWidth, 0)
        compare(control.implicitContentHeight, 0)
        verify(control.implicitWidth > 0)
        verify(control.implicitHeight > 0)
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

    function test_contentItem() {
        let control = createTemporaryObject(contentPane, testCase)
        verify(control)

        compare(control.contentWidth, 100)
        compare(control.contentHeight, 30)
        compare(control.implicitContentWidth, 100)
        compare(control.implicitContentHeight, 30)
        verify(control.implicitWidth > 100)
        verify(control.implicitHeight > 30)
    }

    Component {
        id: contentItemPane
        Pane {
            property string description: ""
            contentItem: ColumnLayout {
                Label {
                    Layout.maximumWidth: 100
                    text: description
                    elide: Label.ElideRight
                }
            }
            Component.onCompleted: {
                description = "Binding loop issue ".repeat(100)
            }
        }
    }

    function test_paneBindingLoop() {
        let control = createTemporaryObject(contentItemPane, testCase)
        verify(control)
        // Wait for content item to be polished
        waitForPolish(control.contentItem)

        compare(control.contentWidth, 100)
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

    function test_press() {
        let control = createTemporaryObject(pressPane, testCase)
        verify(control)

        compare(control.pressCount, 0)
        mouseClick(control)
        compare(control.pressCount, 0)

        control.children[0].enabled = false
        mouseClick(control)
        compare(control.pressCount, 1)
    }
}
