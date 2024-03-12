// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick 2.0

QtObject {
    property int rejectValue: 1
    property int expectedValue: rejectValue + 2;
    property bool wasTestSuccessful: false

    Component.onCompleted: {
        var promise = new Promise(function(resolve, reject) {
            reject(rejectValue);
        });

        promise.then(function() {
            throw new Error("Should never be called")
        }, function(val) {
            return val + 2;
        }).then(function(val) {
            if (val === expectedValue) {
                wasTestSuccessful = true
            }
        }, function() {
            throw new Error("Should never be called")
        });
    }
}
