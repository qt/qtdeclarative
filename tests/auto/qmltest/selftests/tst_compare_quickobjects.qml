// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtTest 1.1

TestCase {
    name: "SelfTests_compare_QuickObjects"
    id: testParent

    Rectangle {
        id: item1
        color: "#000000"
    }
    Rectangle {
        id: item2
        color: "#000000"
    }
    Rectangle {
        id: item3
        color: "#ffffff"
    }

    Component {
        id: item4

        Rectangle {
            color: "#ffffff"
        }
    }

    function test_quickobjects() {
        compare(qtest_compareInternal(item1, item1), true, "Identical QtQuick instances");
        compare(qtest_compareInternal(item1, item3), false, "QtQuick instances with different properties");

        expectFail("", "Unsure if we want this.");
        compare(qtest_compareInternal(item1, item2), true, "QtQuick instances with same properties");

        expectFail("", "Unsure if we want this.");
        compare(qtest_compareInternal(item4.createObject(testParent),
                                      item4.createObject(testParent)), true, "QtQuick dynamic instances");
    }
}
