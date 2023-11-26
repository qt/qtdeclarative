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
    name: "GroupBox"

    Component {
        id: groupBox
        GroupBox { }
    }

    Component {
        id: oneChildBox
        GroupBox {
            Item {
                implicitWidth: 100
                implicitHeight: 30
            }
        }
    }

    Component {
        id: twoChildrenBox
        GroupBox {
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
        id: contentBox
        GroupBox {
            contentItem: Item {
                implicitWidth: 100
                implicitHeight: 30
            }
        }
    }

    function init() {
        failOnWarning(/.?/)
    }

    function test_empty() {
        let control = createTemporaryObject(groupBox, testCase)
        verify(control)

        verify(control.contentItem)
        compare(control.contentWidth, 0)
        compare(control.contentHeight, 0)
        compare(control.implicitContentWidth, 0)
        compare(control.implicitContentHeight, 0)
    }

    function test_oneChild() {
        let control = createTemporaryObject(oneChildBox, testCase)
        verify(control)

        compare(control.contentWidth, 100)
        compare(control.contentHeight, 30)
        compare(control.implicitContentWidth, 100)
        compare(control.implicitContentHeight, 30)
        verify(control.implicitWidth > 100)
        verify(control.implicitHeight > 30)
    }

    function test_twoChildren() {
        let control = createTemporaryObject(twoChildrenBox, testCase)
        verify(control)

        compare(control.contentWidth, 0)
        compare(control.contentHeight, 0)
        compare(control.implicitContentWidth, 0)
        compare(control.implicitContentHeight, 0)
        verify(control.implicitWidth > 0)
        verify(control.implicitHeight > 0)
    }

    function test_contentItem() {
        let control = createTemporaryObject(contentBox, testCase)
        verify(control)

        compare(control.contentWidth, 100)
        compare(control.contentHeight, 30)
        compare(control.implicitContentWidth, 100)
        compare(control.implicitContentHeight, 30)
        verify(control.implicitWidth > 100)
        verify(control.implicitHeight > 30)
    }
}
