// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QtTest 1.1

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

        function skip(msg, file, line) {
            failmsg = "skip:" + msg
        }

        function stringify(str) {
            return str;
        }

        function failOnWarning(msg) {
            failmsg = msg; // use failmsg property for simplicity
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

        caught = false;
        try {
            testCase.verify(true, "foo", "bar")
        } catch (e) {
            compare(e.message, "QtQuickTest::fail")
            compare(functions.failmsg, "More than two arguments given to verify(). Did you mean tryVerify() or tryCompare()?")
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

/*
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
*/
    }

    function test_skip() {
        compare(functions.failmsg, "invalid") // Checks that init() was run

        var caught = false
        try {
            testCase.skip("foo")
        } catch (e) {
            compare(e.message, "QtQuickTest::skip")
            compare(functions.failmsg, "skip:foo")
            caught = true
        }
        verify(caught)

        caught = false
        try {
            testCase.skip()
        } catch (e) {
            compare(e.message, "QtQuickTest::skip")
            compare(functions.failmsg, "skip:")
            caught = true
        }
        verify(caught)
    }

    function test_blacklisted_fail()
    {
        verify(false)
    }

    function test_blacklistWithData_data() {
        return [
            {
                tag: "test1",
                success: true
            },
            {
                tag: "test2",
                success: false
            },
            {
                tag: "test3",
                success: true
            }
        ]
    }

    function test_blacklistWithData(row) {
        verify(row.success)
    }

    function test_failOnWarning() {
        compare(functions.failmsg, "invalid") // Checks that init() was run

        failOnWarning("Warning that will never appear") // shouldn't fail the test

        // without going to C++ or QTestLog introspection, we can kind of only
        // make sure that TestCase does correct job by duck-typing TestResult
        testCase.failOnWarning(undefined)
        compare(functions.failmsg, "")

        testCase.failOnWarning("foobar")
        compare(functions.failmsg, "foobar")

        // one case that is actually testable is whether ignoreWarning()
        // suppresses the failure of failOnWarning()
        var pattern = new RegExp("This warning has to happen")
        var string = "And this one too!"

        failOnWarning(pattern)
        ignoreWarning(pattern)
        console.warn("This warning has to happen!!")

        failOnWarning(string)
        ignoreWarning(string)
        console.warn(string)
    }
}
