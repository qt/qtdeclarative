/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtTest 1.0

TestCase {
    name: "SelfTests"

    // Replace the TestResult functions in "testCase" with hooks
    // that record the events but don't send them to QTestLib.
    QtObject {
        id: functions
        property string failmsg: "cleaned"
        property string actual: ""
        property string expected: ""
        property variant functionsToRun: []

        function fail(msg, file, line) {
            failmsg = msg
        }

        function verify(cond, msg, file, line) {
            if (cond) {
                failmsg = "verify-ok"
                return true
            } else {
                failmsg = msg
                return false
            }
        }

        function compare(success, msg, act, exp, file, line) {
            if (success) {
                failmsg = "compare-ok"
                actual = ""
                expected = ""
                return true
            } else {
                failmsg = msg
                actual = act
                expected = exp
                return false
            }
        }

        function skipSingle(msg, file, line) {
            failmsg = "skipSingle:" + msg
        }

        function skipAll(msg, file, line) {
            failmsg = "skipAll:" + msg
        }
    }

    TestCase {
        id: testCase
        when: false
        optional: true
        qtest_results: functions
    }

    function init() {
        compare(functions.failmsg, "cleaned") // Checks for previous cleanup()
        functions.failmsg = "invalid"
    }

    function cleanup() {
        functions.failmsg = "cleaned"
    }

    function test_fail() {
        compare(functions.failmsg, "invalid") // Checks that init() was run

        var caught = false
        try {
            testCase.fail("foo")
        } catch (e) {
            compare(e.message, "QtQuickTest::fail")
            compare(functions.failmsg, "foo")
            caught = true
        }
        verify(caught)

        caught = false
        try {
            testCase.fail()
        } catch (e) {
            compare(e.message, "QtQuickTest::fail")
            compare(functions.failmsg, "")
            caught = true
        }
        verify(caught)

        caught = false
        try {
            testCase.fail(false)
        } catch (e) {
            compare(e.message, "QtQuickTest::fail")
            compare(functions.failmsg, "false")
            caught = true
        }
        verify(caught)

        caught = false
        try {
            testCase.fail(3)
        } catch (e) {
            compare(e.message, "QtQuickTest::fail")
            compare(functions.failmsg, "3")
            caught = true
        }
        verify(caught)
    }

    function test_verify() {
        compare(functions.failmsg, "invalid") // Checks that init() was run

        try {
            testCase.verify(true)
        } catch (e) {
            fail("verify(true) did not succeed")
        }
        compare(functions.failmsg, "verify-ok")

        var caught = false;
        try {
            testCase.verify(false, "foo")
        } catch (e) {
            compare(e.message, "QtQuickTest::fail")
            compare(functions.failmsg, "foo")
            caught = true
        }
        verify(caught)

        caught = false;
        try {
            testCase.verify(false)
        } catch (e) {
            compare(e.message, "QtQuickTest::fail")
            compare(functions.failmsg, "")
            caught = true
        }
        verify(caught)
    }

    function test_compare() {
        compare(functions.failmsg, "invalid") // Checks that init() was run

        try {
            testCase.compare(23, 23)
        } catch (e) {
            fail("compare(23, 23) did not succeed")
        }
        compare(functions.failmsg, "compare-ok")

        var caught = false;
        try {
            testCase.compare(23, 42, "foo")
        } catch (e) {
            compare(e.message, "QtQuickTest::fail")
            compare(functions.failmsg, "foo")
            compare(functions.actual, "23")
            compare(functions.expected, "42")
            caught = true
        }
        verify(caught)

        caught = false;
        try {
            testCase.compare("abcdef", 42)
        } catch (e) {
            compare(e.message, "QtQuickTest::fail")
            compare(functions.failmsg, "Compared values are not the same")
            compare(functions.actual, "abcdef")
            compare(functions.expected, "42")
            caught = true
        }
        verify(caught)

        caught = false;
        try {
            testCase.compare(Qt.vector3d(1, 2, 3), Qt.vector3d(-1, 2, 3), "x")
        } catch (e) {
            compare(e.message, "QtQuickTest::fail")
            compare(functions.failmsg, "x")
            compare(functions.actual, "Qt.vector3d(1, 2, 3)")
            compare(functions.expected, "Qt.vector3d(-1, 2, 3)")
            caught = true
        }
        verify(caught)

        caught = false;
        try {
            testCase.compare(Qt.vector3d(1, 2, 3), Qt.vector3d(1, -2, 3), "y")
        } catch (e) {
            compare(e.message, "QtQuickTest::fail")
            compare(functions.failmsg, "y")
            compare(functions.actual, "Qt.vector3d(1, 2, 3)")
            compare(functions.expected, "Qt.vector3d(1, -2, 3)")
            caught = true
        }
        verify(caught)

        caught = false;
        try {
            testCase.compare(Qt.vector3d(1, 2, 3), Qt.vector3d(1, 2, -3), "z")
        } catch (e) {
            compare(e.message, "QtQuickTest::fail")
            compare(functions.failmsg, "z")
            compare(functions.actual, "Qt.vector3d(1, 2, 3)")
            compare(functions.expected, "Qt.vector3d(1, 2, -3)")
            caught = true
        }
        verify(caught)

        caught = false;
        try {
            testCase.compare(Qt.vector3d(1, 2, 3), Qt.vector3d(1, 2, 3))
        } catch (e) {
            fail("vector compare did not succeed")
        }
        compare(functions.failmsg, "compare-ok")
    }

    function test_skip() {
        compare(functions.failmsg, "invalid") // Checks that init() was run

        var caught = false
        try {
            testCase.skip("foo")
        } catch (e) {
            compare(e.message, "QtQuickTest::skip")
            compare(functions.failmsg, "skipSingle:foo")
            caught = true
        }
        verify(caught)

        caught = false
        try {
            testCase.skip()
        } catch (e) {
            compare(e.message, "QtQuickTest::skip")
            compare(functions.failmsg, "skipSingle:")
            caught = true
        }
        verify(caught)

        caught = false
        try {
            testCase.skipAll("foo")
        } catch (e) {
            compare(e.message, "QtQuickTest::skip")
            compare(functions.failmsg, "skipAll:foo")
            caught = true
        }
        verify(caught)

        caught = false
        try {
            testCase.skipAll()
        } catch (e) {
            compare(e.message, "QtQuickTest::skip")
            compare(functions.failmsg, "skipAll:")
            caught = true
        }
        verify(caught)
    }
}
