// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
import QtQuick 2.4

QtObject {
    property int callCount: 0
    property bool wasTestSuccessful: false

    property var promise1: new Promise(function() {})
    property var promise2: new Promise(function() {})
    property var promise3: new Promise(function() {})

    Component.onCompleted: {
        promise1.then = null
        console.log(promise1.then)

        // TODO: This assinment works in JS scope but does not work
        // in QML scope
        promise1.then = promise2.then = promise3.then = function(a, b) {
            console.log("then was called")
            callCount++;
        }

        Promise.all([promise1, promise2, promise3])

        console.log("callCount " + callCount)
        wasTestSuccessful = (callCount === 3)
    }
}
