// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

QtObject {
    property QtObject target: null
    // We could just listen to all signals (try { signal.connect(/*...*/) } catch (e))
    // if it weren't for the fact the spy is often declared as a property of the control,
    // which creates a "spyChanged" signal, which leads to an unexpected spyChanged signal
    // emission. However, we don't know what the property will be called, so the signals
    // have to be listed explicitly.
    property var signals: []
    property var expectedSequence: []
    property int sequenceIndex: 0
    property bool sequenceFailure: false
    readonly property bool success: !sequenceFailure && sequenceIndex === expectedSequence.length

    function reset() {
        sequenceIndex = 0
        sequenceFailure = false
    }

    property QtObject __oldTarget: null
    property var __connections: []

    onExpectedSequenceChanged: reset()

    // We may call __setup from onTargetChanged and as we would read the signals property
    // inside __setup, we may be initializing the binding for signals for the first time, which
    // will write the value to the property and trigger onSignalsChanged and call __setup
    // again. One easy way to protect against it is to evaluate those two dependencies upfront
    onTargetChanged: __setup(target, signals)
    onSignalsChanged: __setup(target, signals)

    function __setup(target, signals) {
        if (__oldTarget) {
            __connections.forEach(function (cx) {
                __oldTarget[cx.name].disconnect(cx.method)
            })
            __oldTarget = null
        }

        __connections = []

        if (!!target && !!signals && signals.length > 0) {
            signals.forEach(function (signalName) {
                var handlerName = "on" + signalName.substr(0, 1).toUpperCase() + signalName.substr(1)
                var method = function() { __checkSignal(signalName, arguments) }
                target[handlerName].connect(method)
                __connections.push({ "name": handlerName, "method": method })
            })
            __oldTarget = target
        }
    }

    function __checkSignal(signalName, signalArgs) {
        if (sequenceFailure)
            return;

        if (sequenceIndex >= expectedSequence.length) {
            console.warn("SignalSequenceSpy: Received unexpected signal '" + signalName + "' (none expected).")
            sequenceFailure = true
            return
        }

        var expectedSignal = expectedSequence[sequenceIndex]
        if (typeof expectedSignal === "string") {
            if (expectedSignal === signalName) {
                sequenceIndex++
                return
            }
        } else if (typeof expectedSignal === "object") {
            var expectedSignalData = expectedSignal
            expectedSignal = expectedSignalData[0]
            if (expectedSignal === signalName) {
                var expectedValues = expectedSignalData[1]
                for (var p in expectedValues) {
                    if (target[p] != expectedValues[p]) {
                        console.warn("SignalSequenceSpy: Value mismatch for property '" + p + "' after '" + signalName + "' signal." +
                                     __mismatchValuesFormat(target[p], expectedValues[p]))
                        sequenceFailure = true
                        return
                    }
                }
                sequenceIndex++
                return
            }
        }
        console.warn("SignalSequenceSpy: Received unexpected signal (is \"" + expectedSignal + "\" listed in the signals array?)" +
                     __mismatchValuesFormat(signalName, expectedSignal))
        sequenceFailure = true
    }

    function __mismatchValuesFormat(actual, expected) {
        return "\n    Actual   : " + actual +
               "\n    Expected : " + expected +
               "\n    Sequence index: " + sequenceIndex
    }
}
