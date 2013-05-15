/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtTest 1.0
import "testlogger.js" as TestLogger

Item {
    id: testCase
    visible: false
    TestUtil {
        id:util
    }

    // Name of the test case to prefix the function name in messages.
    property string name

    // Set to true to start the test running.
    property bool when: true

    // Set to true once the test has completed.
    property bool completed: false

    // Set to true when the test is running but not yet complete.
    property bool running: false

    // Set to true if the test doesn't have to run (because some
    // other test failed which this one depends on).
    property bool optional: false

    property bool windowShown: qtest.windowShown

    // Internal private state.  Identifiers prefixed with qtest are reserved.
    property bool qtest_prevWhen: true
    property int qtest_testId: -1
    property bool qtest_componentCompleted : false
    property var qtest_testCaseResult
    property var qtest_results: qtest_results_normal
    TestResult { id: qtest_results_normal }
    property var qtest_events: qtest_events_normal
    TestEvent { id: qtest_events_normal }

    function fail(msg) {
        if (msg === undefined)
            msg = "";
        qtest_results.fail(msg, util.callerFile(), util.callerLine())
        throw new Error("QtQuickTest::fail")
    }

    function qtest_fail(msg, frame) {
        if (msg === undefined)
            msg = "";
        qtest_results.fail(msg, util.callerFile(frame), util.callerLine(frame))
        throw new Error("QtQuickTest::fail")
    }

    function verify(cond, msg) {
        if (msg === undefined)
            msg = "";
        if (!qtest_results.verify(cond, msg, util.callerFile(), util.callerLine()))
            throw new Error("QtQuickTest::fail")
    }

    // Determine what is o.
    // Discussions and reference: http://philrathe.com/articles/equiv
    // Test suites: http://philrathe.com/tests/equiv
    // Author: Philippe Rathé <prathe@gmail.com>
    function qtest_typeof(o) {
        if (typeof o === "undefined") {
            return "undefined";

        // consider: typeof null === object
        } else if (o === null) {
            return "null";

        } else if (o.constructor === String) {
            return "string";

        } else if (o.constructor === Boolean) {
            return "boolean";

        } else if (o.constructor === Number) {

            if (isNaN(o)) {
                return "nan";
            } else {
                return "number";
            }
        // consider: typeof [] === object
        } else if (o instanceof Array) {
            return "array";

        // consider: typeof new Date() === object
        } else if (o instanceof Date) {
            return "date";

        // consider: /./ instanceof Object;
        //           /./ instanceof RegExp;
        //          typeof /./ === "function"; // => false in IE and Opera,
        //                                          true in FF and Safari
        } else if (o instanceof RegExp) {
            return "regexp";

        } else if (typeof o === "object") {
            if ("mapFromItem" in o && "mapToItem" in o) {
                return "declarativeitem";  // @todo improve detection of declarative items
            } else if ("x" in o && "y" in o && "z" in o) {
                return "vector3d"; // Qt 3D vector
            }
            return "object";
        } else if (o instanceof Function) {
            return "function";
        } else {
            return undefined;
        }
    }

    // Test for equality
    // Large parts contain sources from QUnit or http://philrathe.com
    // Discussions and reference: http://philrathe.com/articles/equiv
    // Test suites: http://philrathe.com/tests/equiv
    // Author: Philippe Rathé <prathe@gmail.com>
    function qtest_compareInternal(act, exp) {
        var success = false;
        if (act === exp) {
            success = true; // catch the most you can
        } else if (act === null || exp === null || typeof act === "undefined" || typeof exp === "undefined") {
            success = false; // don't lose time with error prone cases
        } else {
            var typeExp = qtest_typeof(exp), typeAct = qtest_typeof(act)
            if (typeExp !== typeAct) {
                // allow object vs string comparison (e.g. for colors)
                // else break on different types
                if ((typeExp === "string" && (typeAct === "object") || typeAct == "declarativeitem")
                 || ((typeExp === "object" || typeExp == "declarativeitem") && typeAct === "string")) {
                    success = (act == exp)
                }
            } else if (typeExp === "string" || typeExp === "boolean" ||
                       typeExp === "null" || typeExp === "undefined") {
                if (exp instanceof act.constructor || act instanceof exp.constructor) {
                    // to catch short annotaion VS 'new' annotation of act declaration
                    // e.g. var i = 1;
                    //      var j = new Number(1);
                    success = (act == exp)
                } else {
                    success = (act === exp)
                }
            } else if (typeExp === "nan") {
                success = isNaN(act);
            } else if (typeExp === "number") {
                // Use act fuzzy compare if the two values are floats
                if (Math.abs(act - exp) <= 0.00001) {
                    success = true
                }
            } else if (typeExp === "array") {
                success = qtest_compareInternalArrays(act, exp)
            } else if (typeExp === "object") {
                success = qtest_compareInternalObjects(act, exp)
            } else if (typeExp === "declarativeitem") {
                success = qtest_compareInternalObjects(act, exp) // @todo improve comparison of declarative items
            } else if (typeExp === "vector3d") {
                success = (Math.abs(act.x - exp.x) <= 0.00001 &&
                           Math.abs(act.y - exp.y) <= 0.00001 &&
                           Math.abs(act.z - exp.z) <= 0.00001)
            } else if (typeExp === "date") {
                success = (act.valueOf() === exp.valueOf())
            } else if (typeExp === "regexp") {
                success = (act.source === exp.source && // the regex itself
                           act.global === exp.global && // and its modifers (gmi) ...
                           act.ignoreCase === exp.ignoreCase &&
                           act.multiline === exp.multiline)
            }
        }
        return success
    }

    function qtest_compareInternalObjects(act, exp) {
        var i;
        var eq = true; // unless we can proove it
        var aProperties = [], bProperties = []; // collection of strings

        // comparing constructors is more strict than using instanceof
        if (act.constructor !== exp.constructor) {
            return false;
        }

        for (i in act) { // be strict: don't ensures hasOwnProperty and go deep
            aProperties.push(i); // collect act's properties
            if (!qtest_compareInternal(act[i], exp[i])) {
                eq = false;
                break;
            }
        }

        for (i in exp) {
            bProperties.push(i); // collect exp's properties
        }

        // Ensures identical properties name
        return eq && qtest_compareInternal(aProperties.sort(), bProperties.sort());

    }

    function qtest_compareInternalArrays(actual, expected) {
        if (actual.length != expected.length) {
            return false
        }

        for (var i = 0, len = actual.length; i < len; i++) {
            if (!qtest_compareInternal(actual[i], expected[i])) {
                return false
            }
        }

        return true
    }

    function compare(actual, expected, msg) {
        var act = qtest_results.stringify(actual)
        var exp = qtest_results.stringify(expected)

        var success = qtest_compareInternal(actual, expected)
        if (msg === undefined) {
            if (success)
                msg = "COMPARE()"
            else
                msg = "Compared values are not the same"
        }
        if (!qtest_results.compare(success, msg, act, exp, util.callerFile(), util.callerLine())) {
            throw new Error("QtQuickTest::fail")
        }
    }

    function fuzzyCompare(actual, expected, delta, msg) {
        if (delta === undefined)
            qtest_fail("A delta value is required for fuzzyCompare", 2)

        var success = qtest_results.fuzzyCompare(actual, expected, delta)
        if (msg === undefined) {
            if (success)
                msg = "FUZZYCOMPARE()"
            else
                msg = "Compared values are not the same with delta(" + delta + ")"
        }

        if (!qtest_results.compare(success, msg, actual, expected, util.callerFile(), util.callerLine())) {
            throw new Error("QtQuickTest::fail")
        }
    }

    function grabImage(item) {
        return qtest_results.grabImage(item);
    }

    function tryCompare(obj, prop, value, timeout) {
        if (arguments.length == 2) {
            qtest_results.fail("A value is required for tryCompare",
                        util.callerFile(), util.callerLine())
            throw new Error("QtQuickTest::fail")
        }
        if (!timeout)
            timeout = 5000
        if (!qtest_compareInternal(obj[prop], value))
            wait(0)
        var i = 0
        while (i < timeout && !qtest_compareInternal(obj[prop], value)) {
            wait(50)
            i += 50
        }
        var actual = obj[prop]
        var act = qtest_results.stringify(actual)
        var exp = qtest_results.stringify(value)
        var success = qtest_compareInternal(actual, value)
        if (!qtest_results.compare(success, "property " + prop, act, exp, util.callerFile(), util.callerLine()))
            throw new Error("QtQuickTest::fail")
    }

    function skip(msg) {
        if (msg === undefined)
            msg = ""
        qtest_results.skip(msg, util.callerFile(), util.callerLine())
        throw new Error("QtQuickTest::skip")
    }

    function expectFail(tag, msg) {
        if (tag === undefined) {
            warn("tag argument missing from expectFail()")
            tag = ""
        }
        if (msg === undefined) {
            warn("message argument missing from expectFail()")
            msg = ""
        }
        if (!qtest_results.expectFail(tag, msg, util.callerFile(), util.callerLine()))
            throw new Error("QtQuickTest::expectFail")
    }

    function expectFailContinue(tag, msg) {
        if (tag === undefined) {
            warn("tag argument missing from expectFailContinue()")
            tag = ""
        }
        if (msg === undefined) {
            warn("message argument missing from expectFailContinue()")
            msg = ""
        }
        if (!qtest_results.expectFailContinue(tag, msg, util.callerFile(), util.callerLine()))
            throw new Error("QtQuickTest::expectFail")
    }

    function warn(msg) {
        if (msg === undefined)
            msg = ""
        qtest_results.warn(msg, util.callerFile(), util.callerLine());
    }

    function ignoreWarning(msg) {
        if (msg === undefined)
            msg = ""
        qtest_results.ignoreWarning(msg)
    }

    function wait(ms) {
        qtest_results.wait(ms)
    }

    function waitForRendering(item, timeout) {
        if (timeout === undefined)
            timeout = 5000
        return qtest_results.waitForRendering(item, timeout)
    }

    function sleep(ms) {
        qtest_results.sleep(ms)
    }

    function keyPress(key, modifiers, delay) {
        if (modifiers === undefined)
            modifiers = Qt.NoModifier
        if (delay == undefined)
            delay = -1
        if (typeof(key) == "string" && key.length == 1) {
            if (!qtest_events.keyPressChar(key, modifiers, delay))
                qtest_fail("window not shown", 2)
        } else {
            if (!qtest_events.keyPress(key, modifiers, delay))
                qtest_fail("window not shown", 2)
        }
    }

    function keyRelease(key, modifiers, delay) {
        if (modifiers === undefined)
            modifiers = Qt.NoModifier
        if (delay == undefined)
            delay = -1
        if (typeof(key) == "string" && key.length == 1) {
            if (!qtest_events.keyReleaseChar(key, modifiers, delay))
                qtest_fail("window not shown", 2)
        } else {
            if (!qtest_events.keyRelease(key, modifiers, delay))
                qtest_fail("window not shown", 2)
        }
    }

    function keyClick(key, modifiers, delay) {
        if (modifiers === undefined)
            modifiers = Qt.NoModifier
        if (delay == undefined)
            delay = -1
        if (typeof(key) == "string" && key.length == 1) {
            if (!qtest_events.keyClickChar(key, modifiers, delay))
                qtest_fail("window not shown", 2)
        } else {
            if (!qtest_events.keyClick(key, modifiers, delay))
                qtest_fail("window not shown", 2)
        }
    }

    function mousePress(item, x, y, button, modifiers, delay) {
        if (button === undefined)
            button = Qt.LeftButton
        if (modifiers === undefined)
            modifiers = Qt.NoModifier
        if (delay == undefined)
            delay = -1
        if (!qtest_events.mousePress(item, x, y, button, modifiers, delay))
            qtest_fail("window not shown", 2)
    }

    function mouseRelease(item, x, y, button, modifiers, delay) {
        if (button === undefined)
            button = Qt.LeftButton
        if (modifiers === undefined)
            modifiers = Qt.NoModifier
        if (delay == undefined)
            delay = -1
        if (!qtest_events.mouseRelease(item, x, y, button, modifiers, delay))
            qtest_fail("window not shown", 2)
    }

    function mouseDrag(item, x, y, dx, dy, button, modifiers, delay) {
        if (item.x === undefined || item.y === undefined)
            return
        if (button === undefined)
            button = Qt.LeftButton
        if (modifiers === undefined)
            modifiers = Qt.NoModifier
        if (delay == undefined)
            delay = -1

        // Divide dx and dy to have intermediate mouseMove while dragging
        // Fractions of dx/dy need be superior to the dragThreshold
        // to make the drag works though
        var ddx = Math.round(dx/3)
        if (ddx < (util.dragThreshold + 1))
            ddx = 0
        var ddy = Math.round(dy/3)
        if (ddy < (util.dragThreshold + 1))
            ddy = 0

        mousePress(item, x, y, button, modifiers, delay)
        //trigger dragging
        mouseMove(item, x + util.dragThreshold + 1, y + util.dragThreshold + 1, delay, button)
        if (ddx > 0 || ddy > 0) {
            mouseMove(item, x + ddx, y + ddy, delay, button)
            mouseMove(item, x + 2*ddx, y + 2*ddy, delay, button)
        }
        mouseMove(item, x + dx, y + dy, delay, button)
        mouseRelease(item, x + dx, y + dy, button, modifiers, delay)
    }

    function mouseClick(item, x, y, button, modifiers, delay) {
        if (button === undefined)
            button = Qt.LeftButton
        if (modifiers === undefined)
            modifiers = Qt.NoModifier
        if (delay == undefined)
            delay = -1
        if (!qtest_events.mouseClick(item, x, y, button, modifiers, delay))
            qtest_fail("window not shown", 2)
    }

    function mouseDoubleClick(item, x, y, button, modifiers, delay) {
        if (button === undefined)
            button = Qt.LeftButton
        if (modifiers === undefined)
            modifiers = Qt.NoModifier
        if (delay == undefined)
            delay = -1
        if (!qtest_events.mouseDoubleClick(item, x, y, button, modifiers, delay))
            qtest_fail("window not shown", 2)
    }

    function mouseMove(item, x, y, delay, buttons) {
        if (delay == undefined)
            delay = -1
        if (buttons == undefined)
            buttons = Qt.NoButton
        if (!qtest_events.mouseMove(item, x, y, delay, buttons))
            qtest_fail("window not shown", 2)
    }

    function mouseWheel(item, x, y, xDelta, yDelta, buttons, modifiers, delay) {
        if (delay == undefined)
            delay = -1
        if (buttons == undefined)
            buttons = Qt.NoButton
        if (modifiers === undefined)
            modifiers = Qt.NoModifier
        if (xDelta == undefined)
            xDelta = 0
        if (yDelta == undefined)
            yDelta = 0
        if (!qtest_events.mouseWheel(item, x, y, buttons, modifiers, xDelta, yDelta, delay))
            qtest_fail("window not shown", 2)
   }


    // Functions that can be overridden in subclasses for init/cleanup duties.
    function initTestCase() {}
    function cleanupTestCase() {}
    function init() {}
    function cleanup() {}

    function qtest_runInternal(prop, arg) {
        try {
            qtest_testCaseResult = testCase[prop](arg)
        } catch (e) {
            qtest_testCaseResult = []
            if (e.message.indexOf("QtQuickTest::") != 0) {
                // Test threw an unrecognized exception - fail.
                qtest_results.fail("Uncaught exception: " + e.message,
                             e.fileName, e.lineNumber)
            }
        }
        return !qtest_results.failed
    }

    function qtest_runFunction(prop, arg) {
        qtest_runInternal("init")
        if (!qtest_results.skipped) {
            qtest_runInternal(prop, arg)
            qtest_results.finishTestData()
            qtest_runInternal("cleanup")
            qtest_results.finishTestDataCleanup()
        }
    }

    function qtest_runBenchmarkFunction(prop, arg) {
        qtest_results.startMeasurement()
        do {
            qtest_results.beginDataRun()
            do {
                // Run the initialization function.
                qtest_runInternal("init")
                if (qtest_results.skipped)
                    break

                // Execute the benchmark function.
                if (prop.indexOf("benchmark_once_") != 0)
                    qtest_results.startBenchmark(TestResult.RepeatUntilValidMeasurement, qtest_results.dataTag)
                else
                    qtest_results.startBenchmark(TestResult.RunOnce, qtest_results.dataTag)
                while (!qtest_results.isBenchmarkDone()) {
                    var success = qtest_runInternal(prop, arg)
                    qtest_results.finishTestData()
                    if (!success)
                        break
                    qtest_results.nextBenchmark()
                }
                qtest_results.stopBenchmark()

                // Run the cleanup function.
                qtest_runInternal("cleanup")
                qtest_results.finishTestDataCleanup()
            } while (!qtest_results.measurementAccepted())
            qtest_results.endDataRun()
        } while (qtest_results.needsMoreMeasurements())
    }

    function qtest_run() {
        if (util.printAvailableFunctions) {
            completed = true
            return
        }

        if (TestLogger.log_start_test()) {
            qtest_results.reset()
            qtest_results.testCaseName = name
            qtest_results.startLogging()
        } else {
            qtest_results.testCaseName = name
        }
        running = true

        // Check the run list to see if this class is mentioned.
        var functionsToRun = qtest_results.functionsToRun
        if (functionsToRun.length > 0) {
            var found = false
            var list = []
            if (name.length > 0) {
                var prefix = name + "::"
                for (var index in functionsToRun) {
                    if (functionsToRun[index].indexOf(prefix) == 0) {
                        list.push(functionsToRun[index])
                        found = true
                    }
                }
            }
            if (!found) {
                completed = true
                if (!TestLogger.log_complete_test(qtest_testId)) {
                    qtest_results.stopLogging()
                    Qt.quit()
                }
                qtest_results.testCaseName = ""
                return
            }
            functionsToRun = list
        }

        // Run the initTestCase function.
        qtest_results.functionName = "initTestCase"
        var runTests = true
        if (!qtest_runInternal("initTestCase"))
            runTests = false
        qtest_results.finishTestData()
        qtest_results.finishTestDataCleanup()
        qtest_results.finishTestFunction()

        // Run the test methods.
        var testList = []
        if (runTests) {
            for (var prop in testCase) {
                if (prop.indexOf("test_") != 0 && prop.indexOf("benchmark_") != 0)
                    continue
                var tail = prop.lastIndexOf("_data");
                if (tail != -1 && tail == (prop.length - 5))
                    continue
                testList.push(prop)
            }
            testList.sort()
        }
        var checkNames = (functionsToRun.length > 0)
        for (var index in testList) {
            var prop = testList[index]
            var datafunc = prop + "_data"
            var isBenchmark = (prop.indexOf("benchmark_") == 0)
            if (checkNames) {
                var index = functionsToRun.indexOf(name + "::" + prop)
                if (index < 0)
                    continue
                functionsToRun.splice(index, 1)
            }
            qtest_results.functionName = prop

            if (!(datafunc in testCase))
                datafunc = "init_data";

            if (datafunc in testCase) {
                if (qtest_runInternal(datafunc)) {
                    var table = qtest_testCaseResult
                    var haveData = false
                    qtest_results.initTestTable()
                    for (var index in table) {
                        haveData = true
                        var row = table[index]
                        if (!row.tag)
                            row.tag = "row " + index    // Must have something
                        qtest_results.dataTag = row.tag
                        if (isBenchmark)
                            qtest_runBenchmarkFunction(prop, row)
                        else
                            qtest_runFunction(prop, row)
                        qtest_results.dataTag = ""
                    }
                    if (!haveData) {
                        if (datafunc === "init_data")
                           qtest_runFunction(prop, null, isBenchmark)
                        else
                           qtest_results.warn("no data supplied for " + prop + "() by " + datafunc + "()"
                                            , util.callerFile(), util.callerLine());
                    }
                    qtest_results.clearTestTable()
                }
            } else if (isBenchmark) {
                qtest_runBenchmarkFunction(prop, null, isBenchmark)
            } else {
                qtest_runFunction(prop, null, isBenchmark)
            }
            qtest_results.finishTestFunction()
            // wait(0) will call processEvents() so objects marked for deletion
            // in the test function will be deleted.
            wait(0)
            qtest_results.skipped = false
        }

        // Run the cleanupTestCase function.
        qtest_results.skipped = false
        qtest_results.functionName = "cleanupTestCase"
        qtest_runInternal("cleanupTestCase")

        // Complain about missing functions that we were supposed to run.
        if (functionsToRun.length > 0)
            qtest_results.fail("Could not find functions: " + functionsToRun, "", 0)

        // Clean up and exit.
        running = false
        completed = true
        qtest_results.finishTestData()
        qtest_results.finishTestDataCleanup()
        qtest_results.finishTestFunction()
        qtest_results.functionName = ""

        // Stop if there are no more tests to be run.
        if (!TestLogger.log_complete_test(qtest_testId)) {
            qtest_results.stopLogging()
            Qt.quit()
        }
        qtest_results.testCaseName = ""
    }

    onWhenChanged: {
        if (when != qtest_prevWhen) {
            qtest_prevWhen = when
            if (when && !completed && !running && qtest_componentCompleted)
                qtest_run()
        }
    }

    onOptionalChanged: {
        if (!completed) {
            if (optional)
                TestLogger.log_optional_test(qtest_testId)
            else
                TestLogger.log_mandatory_test(qtest_testId)
        }
    }


    Component.onCompleted: {
        qtest.hasTestCase = true;
        qtest_componentCompleted = true;

        if (util.printAvailableFunctions) {
            var testList = []
            for (var prop in testCase) {
                if (prop.indexOf("test_") != 0 && prop.indexOf("benchmark_") != 0)
                    continue
                var tail = prop.lastIndexOf("_data");
                if (tail != -1 && tail == (prop.length - 5))
                    continue
                // Note: cannot run functions in TestCase elements
                // that lack a name.
                if (name.length > 0)
                    testList.push(name + "::" + prop + "()")
            }
            testList.sort()
            for (var index in testList)
                console.log(testList[index])
            return
        }
        qtest_testId = TestLogger.log_register_test(name)
        if (optional)
            TestLogger.log_optional_test(qtest_testId)
        qtest_prevWhen = when
        if (when && !completed && !running)
            qtest_run()
    }
}
