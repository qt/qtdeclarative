// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
import QtQuick 2.0

QtObject {
    property int resolveValue: 5

    property bool promise1WasResolved: false
    property bool promise2WasResolved: false
    property bool promise3WasResolved: false
    property bool promise4WasResolved: true

    property bool wasTestSuccessful: promise1WasResolved && promise2WasResolved &&
                                    promise3WasResolved && promise4WasResolved

    // TODO: Should this work as well?
    // property Promise promise
    property var promise1: new Promise(function (resolve, reject) {
        resolve(resolveValue)
    })
    property var promise2: new Promise(function (resolve, reject) {
        resolve(resolveValue)
    })
    property var promise3: new Promise(function (resolve, reject) {
        resolve(resolveValue)
    })
    property var promise4: new Promise(function (resolve, reject) {
        resolve(resolveValue)
    })

    Component.onCompleted: {
        promise1.then().then(function (result) {
            promise1WasResolved = (result === resolveValue);
        }, function() {
            throw new Error("Should never be called")
        })
        promise2.then(3, 5).then(function (result) {
            promise2WasResolved = (result === resolveValue);
        }, function() {
            throw new Error("Should never be called")
        })
        promise3.then(null, function() {
            throw new Error("Should never be called")
        }).then(function (result) {
            promise3WasResolved = (result === resolveValue);
        }, function() {
            throw new Error("Should never be called")
        })
        /*
        promise4.then(undefined, undefined).then(function (result) {
            promise4WasResolved = (result === resolveValue);
        }, function() {
            throw new Error("Should never be called")
        })
        */
    }
}
