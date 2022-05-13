// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
import QtQuick 2.0

QtObject {
    property int rejectValue: 5

    property bool promise1WasRejected: false
    property bool promise2WasRejected: false

    property bool wasTestSuccessful: promise1WasRejected && promise2WasRejected

    // TODO: Should this work as well?
    // property Promise promise
    property var promise1: new Promise(function (resolve, reject) {
        reject(rejectValue)
    })
    property var promise2: new Promise(function (resolve, reject) {
        reject(rejectValue)
    })

    Component.onCompleted: {
        promise1.then().then(function() {
            promise1WasRejected = false
            throw new Error("Should never be called")
        }, function (result) {
            promise1WasRejected = (result === rejectValue);
        })
        promise2.then(3, 5).then(function() {
            promise2WasRejected = false
            throw new Error("Should never be called")
        }, function (result) {
            promise2WasRejected = (result === rejectValue);
        })
    }
}
