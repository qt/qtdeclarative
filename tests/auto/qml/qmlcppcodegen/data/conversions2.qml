import QtQuick
import QtTest

Item {
    id: spy
    visible: false

    Rectangle {
        id: rect
    }

    TestUtil {
        id: util
    }

    property var target: null
    property string signalName: ""
    readonly property alias count: spy.qtest_count
    readonly property alias valid:spy.qtest_valid
    readonly property alias signalArguments:spy.qtest_signalArguments

    function clearRectGradient() {
        rect.gradient = null;
    }

    function clear() {
        qtest_count = 0
        qtest_expectedCount = 0
        qtest_signalArguments = []
    }

    function wait(timeout) {
        if (timeout === undefined)
            timeout = 5000
        var expected = ++qtest_expectedCount
        var i = 0
        while (i < timeout && qtest_count < expected) {
            qtest_results.wait(50)
            i += 50
        }
        var success = (qtest_count >= expected)
        if (!qtest_results.verify(success, "wait for signal " + signalName, util.callerFile(), util.callerLine()))
            return false;
    }

    TestResult { id: qtest_results }

    onTargetChanged: {
        qtest_update()
    }
    onSignalNameChanged: {
        qtest_update()
    }

    property var qtest_prevTarget: null
    property string qtest_prevSignalName: ""
    property int qtest_expectedCount: 0
    property var qtest_signalArguments:[]
    property int qtest_count: 0
    property bool qtest_valid:false

    function qtest_update() {
        if (qtest_prevTarget != null) {
            var prevHandlerName = qtest_signalHandlerName(qtest_prevSignalName)
            var prevFunc = qtest_prevTarget[prevHandlerName]
            if (prevFunc)
                prevFunc.disconnect(spy.qtest_activated)
            qtest_prevTarget = null
            qtest_prevSignalName = ""
        }
        if (target != null && signalName != "") {
            var func = target[signalName]
            if (typeof func !== "function") {
                func = target[qtest_signalHandlerName(signalName)]
            }
            if (func === undefined) {
                spy.qtest_valid = false
                console.log("Signal '" + signalName + "' not found")
            } else {
                qtest_prevTarget = target
                qtest_prevSignalName = signalName
                func.connect(spy.qtest_activated)
                spy.qtest_valid = true
                spy.qtest_signalArguments = []
            }
        } else {
            spy.qtest_valid = false
        }
    }

    function qtest_activated() {
        ++qtest_count
        spy.qtest_signalArguments[spy.qtest_signalArguments.length] = [1, 2, 3]
    }

    function qtest_signalHandlerName(sn) {
        // Warning: to not test for signal handlers like this in actual code.
        // Use the helper methods in QQmlSignalNames instead.
        if (sn.substr(0, 2) === "on" && sn[2] === sn[2].toUpperCase())
            return sn
        return "on" + sn.substr(0, 1).toUpperCase() + sn.substr(1)
    }
}
