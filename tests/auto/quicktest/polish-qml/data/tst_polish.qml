/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
