// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
import QtQuick 2.4

QtObject {
    property int resolveValue: 33

    property var delayedExecutor: Timer {
        interval: 0
        property var executor: null
        onTriggered: {
            if (executor) {
                executor();
            }
        }
    }

    property var promise: new Promise(function (resolve, reject) {
        delayedExecutor.executor = function() {
            resolve(resolveValue + 5)
        }
        delayedExecutor.restart();
    })

    property var resolvedPromiseArray: [promise, Promise.resolve(resolveValue)]
    property bool wasTestSuccessful: false

    Component.onCompleted: {
        Promise.race(resolvedPromiseArray).then(function(value) {
            if (value !== resolveValue) {
                return;
            }

            wasTestSuccessful = true
        }, function() {
            throw new Error("Should never be called")
        })
    }
}
