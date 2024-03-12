// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick 2.0

QtObject {
    property int resolveValue: 5

    property bool was1stCallSucessfull: false
    property bool was2ndCallSucessfull: false

    property bool wasTestSuccessful: was1stCallSucessfull && was2ndCallSucessfull

    property var promise: new Promise(function (resolve, reject) {
        resolve(resolveValue)
    })

    Component.onCompleted: {
        promise.then(function (result) {
            was1stCallSucessfull = (result === resolveValue);
        }, function() {
            throw new Error("Should never be called")
        })
        promise.then(function (result) {
            was2ndCallSucessfull = (result === resolveValue);
        }, function() {
            throw new Error("Should never be called")
        })
    }
}
