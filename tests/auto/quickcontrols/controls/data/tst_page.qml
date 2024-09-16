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
    name: "Page"

    Component {
        id: page
        Page { }
    }

    Component {
        id: oneChildPage
        Page {
            Item {
                implicitWidth: 100
                implicitHeight: 30
            }
        }
    }

    Component {
        id: twoChildrenPage
        Page {
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
        id: contentPage
        Page {
            contentItem: Item {
                implicitWidth: 100
                implicitHeight: 30
            }
        }
    }

    Component {
        id: headerFooterPage
        Page {
            header: ToolBar { }
            footer: ToolBar { }
            contentItem: Item {
                implicitWidth: 100
                implicitHeight: 30
            }
        }
    }

    Component {
        id: toolBar
        ToolBar { }
    }

    function init() {
        failOnWarning(/.?/)
    }

    function test_defaults() {
        let control = createTemporaryObject(page, testCase)
        verify(control)

        verify(control.contentItem)
        compare(control.header, null)
        compare(control.footer, null)
    }

    function test_empty() {
        let control = createTemporaryObject(page, testCase)
        verify(control)

        verify(control.contentItem)
        compare(control.contentWidth, 0)
        compare(control.contentHeight, 0)
        compare(control.implicitContentWidth, 0)
        compare(control.implicitContentHeight, 0)
    }

    function test_oneChild() {
        let control = createTemporaryObject(oneChildPage, testCase)
        verify(control)

        compare(control.contentWidth, 100)
        compare(control.contentHeight, 30)
        compare(control.implicitContentWidth, 100)
        compare(control.implicitContentHeight, 30)
        compare(control.implicitWidth, 100 + control.leftPadding + control.rightPadding)
        compare(control.implicitHeight, 30 + control.topPadding + control.bottomPadding)
    }

    function test_twoChildren() {
        let control = createTemporaryObject(twoChildrenPage, testCase)
        verify(control)

        compare(control.contentWidth, 0)
        compare(control.contentHeight, 0)
        compare(control.implicitContentWidth, 0)
        compare(control.implicitContentHeight, 0)
        compare(control.implicitWidth, Math.max(control.leftPadding + control.rightPadding,
            control.background ? control.background.implicitWidth : 0))
        compare(control.implicitHeight, Math.max(control.topPadding + control.bottomPadding,
            control.background ? control.background.implicitHeight : 0))
    }

    function test_contentItem() {
        let control = createTemporaryObject(contentPage, testCase)
        verify(control)

        compare(control.contentWidth, 100)
        compare(control.contentHeight, 30)
        compare(control.implicitContentWidth, 100)
        compare(control.implicitContentHeight, 30)
        compare(control.implicitWidth, 100 + control.leftPadding + control.rightPadding)
        compare(control.implicitHeight, 30 + control.topPadding + control.bottomPadding)
    }

    function test_layout() {
        let control = createTemporaryObject(page, testCase, {width: 100, height: 100})
        verify(control)

        compare(control.width, 100)
        compare(control.height, 100)
        compare(control.contentItem.width, control.width)
        compare(control.contentItem.height, control.height)

        control.header = toolBar.createObject(control)
        compare(control.header.width, control.width)
        verify(control.header.height > 0)
        compare(control.contentItem.width, control.width)
        compare(control.contentItem.height, control.height - control.header.height)

        control.footer = toolBar.createObject(control)
        compare(control.footer.width, control.width)
        verify(control.footer.height > 0)
        compare(control.contentItem.width, control.width)
        compare(control.contentItem.height, control.height - control.header.height - control.footer.height)

        control.topPadding = 9
        control.leftPadding = 2
        control.rightPadding = 6
        control.bottomPadding = 7

        compare(control.header.x, 0)
        compare(control.header.y, 0)
        compare(control.header.width, control.width)
        verify(control.header.height > 0)

        compare(control.footer.x, 0)
        compare(control.footer.y, control.height - control.footer.height)
        compare(control.footer.width, control.width)
        verify(control.footer.height > 0)

        compare(control.contentItem.x, control.leftPadding)
        compare(control.contentItem.y, control.topPadding + control.header.height)
        compare(control.contentItem.width, control.availableWidth)
        compare(control.contentItem.height, control.availableHeight - control.header.height - control.footer.height)

        control.header.visible = false
        compare(control.contentItem.x, control.leftPadding)
        compare(control.contentItem.y, control.topPadding)
        compare(control.contentItem.width, control.availableWidth)
        compare(control.contentItem.height, control.availableHeight - control.footer.height)

        control.footer.visible = false
        compare(control.contentItem.x, control.leftPadding)
        compare(control.contentItem.y, control.topPadding)
        compare(control.contentItem.width, control.availableWidth)
        compare(control.contentItem.height, control.availableHeight)

        control.contentItem.implicitWidth = 50
        control.contentItem.implicitHeight = 60
        compare(control.implicitWidth, control.contentItem.implicitWidth + control.leftPadding + control.rightPadding)
        compare(control.implicitHeight, control.contentItem.implicitHeight + control.topPadding + control.bottomPadding)

        control.header.visible = true
        compare(control.implicitHeight, control.contentItem.implicitHeight + control.topPadding + control.bottomPadding
                                      + control.header.implicitHeight + control.spacing)

        control.footer.visible = true
        compare(control.implicitHeight, control.contentItem.implicitHeight + control.topPadding + control.bottomPadding
                                      + control.header.implicitHeight + control.footer.implicitHeight + 2 * control.spacing)

        control.header.implicitWidth = 150
        compare(control.implicitWidth, control.header.implicitWidth)

        control.footer.implicitWidth = 160
        compare(control.implicitWidth, control.footer.implicitWidth)

        control.contentItem.implicitWidth = 170
        compare(control.implicitWidth, control.contentItem.implicitWidth + control.leftPadding + control.rightPadding)
    }

    function test_spacing_data() {
        return [
            { tag: "content", header: false, content: true, footer: false },
            { tag: "header,content", header: true, content: true, footer: false },
            { tag: "content,footer", header: false, content: true, footer: true },
            { tag: "header,content,footer", header: true, content: true, footer: true },
            { tag: "header,footer", header: true, content: false, footer: true },
            { tag: "header", header: true, content: false, footer: false },
            { tag: "footer", header: false, content: false, footer: true },
        ]
    }

    function test_spacing(data) {
        let control = createTemporaryObject(page, testCase, {spacing: 20, width: 100, height: 100})
        verify(control)

        control.contentItem.visible = data.content
        control.header = toolBar.createObject(control.contentItem, {visible: data.header})
        control.footer = toolBar.createObject(control.contentItem, {visible: data.footer})

        compare(control.header.x, 0)
        compare(control.header.y, 0)
        compare(control.header.width, control.width)
        verify(control.header.height > 0)

        compare(control.footer.x, 0)
        compare(control.footer.y, control.height - control.footer.height)
        compare(control.footer.width, control.width)
        verify(control.footer.height > 0)

        compare(control.contentItem.x, control.leftPadding)
        compare(control.contentItem.y, control.topPadding + (data.header ? control.header.height + control.spacing : 0))
        compare(control.contentItem.width, control.availableWidth)
        compare(control.contentItem.height, control.availableHeight
                                            - (data.header ? control.header.height + control.spacing : 0)
                                            - (data.footer ? control.footer.height + control.spacing : 0))
    }

    function test_headerFooter() {
        let control = createTemporaryObject(headerFooterPage, testCase, {width: 100, height: 100})
        verify(control)

        compare(control.width, 100)
        compare(control.height, 100)

        verify(control.header)
        compare(control.header.x, 0)
        compare(control.header.y, 0)
        compare(control.header.width, control.width)
        verify(control.header.height > 0)

        verify(control.footer)
        compare(control.footer.x, 0)
        compare(control.footer.y, control.height - control.footer.height)
        compare(control.footer.width, control.width)
        verify(control.footer.height > 0)

        compare(control.contentItem.x, 0)
        compare(control.contentItem.y, control.header.height)
        compare(control.contentItem.width, control.width)
        compare(control.contentItem.height, control.height - control.header.height - control.footer.height)

        // swap places and make sure geometry is updated correctly
        const oldHeader = control.header
        const oldFooter = control.footer
        // reset both first, so one item does not end up in two places at once
        control.header = null
        control.footer = null
        control.header = oldFooter
        control.footer = oldHeader
        verify(control.header.visible)
        verify(control.footer.visible)
        compare(control.header.y, 0)
        compare(control.footer.y, control.height - control.footer.height)
    }
}
