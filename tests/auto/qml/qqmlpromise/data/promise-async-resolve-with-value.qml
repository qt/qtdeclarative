// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick 2.4

QtObject {
    property int resolveValue: 5
    property bool wasTestSuccessful: false


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
            resolve(resolveValue)
        }
        delayedExecutor.restart();
    })

    Component.onCompleted: {
        promise.then(function(value) {
            if (value === resolveValue) {
                wasTestSuccessful = true
            }
        }, function() {
            throw new Error("Should never be called")
        })
    }
}
