/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.5

QtObject {
    property QtObject target: null
    property var expectedSequence: []
    property int sequenceIndex: 0
    property bool sequenceFailure: false
    readonly property bool success: sequenceIndex === expectedSequence.length
                                    && !sequenceFailure

    function reset() {
        sequenceIndex = 0
        sequenceFailure = false
    }

    function checkSignal(signalName) {
        if (sequenceFailure)
            return;

        if (!expectedSequence.length || sequenceIndex >= expectedSequence.length) {
            console.warn("ControlSpy: received unexpected signal '" + signalName + "'.")
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
            expectedSignal = expectedSignal[0]
            if (expectedSignal === signalName) {
                var expectedValues = expectedSequence[sequenceIndex][1]
                for (var p in expectedValues)
                    if (target[p] != expectedValues[p]) {
                        console.warn("ControlSpy: property value mismatch for '" + p + "'" +
                                     " after '" + signalName + "' signal. " +
                                     "Expected: " + expectedValues[p] + ", actual: " + control[p] +".")
                        sequenceFailure = true
                        return
                    }
                sequenceIndex++
                return
            }
        }
        console.warn("ControlSpy: signal names mismatch. " +
                     "Expected: " + expectedSignal + ", received: " + signalName +".")
        sequenceFailure = true
    }

    onExpectedSequenceChanged: reset()
}
