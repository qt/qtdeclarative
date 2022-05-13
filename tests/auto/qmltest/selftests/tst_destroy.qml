// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.1
import QtTest 1.1

TestCase {
    id: testCase
    width: 100
    height: 100
    name: "SelfTests_Destroy"

    function test_a_QTBUG_30523() {
        compare(testCase.children.length, 0)
        var tmp = Qt.createQmlObject('import QtQuick 2.1; Rectangle {width: 20; height: 20;}', testCase, '')
        compare(testCase.children.length, 1)
        tmp.destroy()
    }

    function test_b_QTBUG_30523() {
        // The object created in test above should be deleted
        compare(testCase.children.length, 0)
    }

    function test_c_QTBUG_42185_data() {
   // Adding dummy data objects for the purpose of calling multiple times the test function
   // and checking that the created object (tmp) is destroyed as expected between each data run.
        return [{tag: "test 0"}, {tag: "test 1"}, {tag: "test 2"},];
    }

    function test_c_QTBUG_42185() {
        compare(testCase.children.length, 0)
        var tmp = Qt.createQmlObject('import QtQuick 2.1; Rectangle {width: 20; height: 20;}', testCase, '')
        compare(testCase.children.length, 1)
        tmp.destroy()
    }

    function test_d_QTBUG_42185() {
        // The object created in test above should be deleted
        compare(testCase.children.length, 0)
    }
}
