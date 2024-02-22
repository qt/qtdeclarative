// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick 2.4

QtObject {
    property int rejectValue: 5
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
            reject(rejectValue)
        }
        delayedExecutor.restart();
    })

    Component.onCompleted: {
        promise.then(function() {
            throw new Error("Should never be called")
        }, function(value) {
            if (value === rejectValue) {
                wasTestSuccessful = true
            }
        })
    }
}
