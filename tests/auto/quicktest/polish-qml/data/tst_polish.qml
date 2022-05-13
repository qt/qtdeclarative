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

    function test_polish()
    {
        var item = createTemporaryObject(customItemComponent, testCase)
        verify(item)

        item.polishMe()
        verify(isPolishScheduled(item))
        verify(item.isPolishScheduled())

        verify(waitForItemPolished(item))
        verify(item.wasUpdatePolishCalled())

        // TODO: test failure conditions when https://bugreports.qt.io/browse/QTBUG-72351 is fixed
//        expectFail("", "Not a valid item")
//        isPolishScheduled(null)
    }
}
