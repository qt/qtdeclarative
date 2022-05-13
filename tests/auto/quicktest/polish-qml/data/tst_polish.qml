// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.13
import QtTest 1.13

import Test 1.0

TestCase {
    id: testCase
    name: "polish-qml"
    when: windowShown

    Component {
        id: customItemComponent
        CustomItem {}
    }

    function test_polish() {
        let item = createTemporaryObject(customItemComponent, testCase)
        verify(item)

        // Test item polish API.
        item.polishMe()
        verify(isPolishScheduled(item))
        verify(item.isPolishScheduled())
        verify(!item.wasUpdatePolishCalled())

        verify(waitForPolish(item))
        verify(item.wasUpdatePolishCalled())

        // TODO: test failure conditions when https://bugreports.qt.io/browse/QTBUG-72351 is fixed
//        expectFail("", "Not a valid item")
//        isPolishScheduled(null)

        // Test window polish API.
        let window = Window.window
        item.clearUpdatePolishCalled()
        item.polishMe()
        verify(isPolishScheduled(window))
        verify(item.isPolishScheduled())
        verify(!item.wasUpdatePolishCalled())

        verify(waitForPolish(window))
        verify(item.wasUpdatePolishCalled())
    }

    Component {
        id: customChildItemComponent
        Item {
            CustomItem {}
        }
    }

    function test_childPolish() {
        let root = createTemporaryObject(customChildItemComponent, testCase)
        verify(root)

        let child = root.children[0]
        verify(child)

        // Test item polish API.
        child.polishMe()
        verify(isPolishScheduled(child))
        verify(child.isPolishScheduled())
        verify(!child.wasUpdatePolishCalled())

        verify(waitForPolish(child))
        verify(child.wasUpdatePolishCalled())

        // TODO: test failure conditions when https://bugreports.qt.io/browse/QTBUG-72351 is fixed
//        expectFail("", "Not a valid item")
//        isPolishScheduled(null)

        // Test window polish API.
        let window = Window.window
        child.clearUpdatePolishCalled()
        child.polishMe()
        verify(isPolishScheduled(window))
        verify(child.isPolishScheduled())
        verify(!child.wasUpdatePolishCalled())

        verify(waitForPolish(window))
        verify(child.wasUpdatePolishCalled())
    }
}
