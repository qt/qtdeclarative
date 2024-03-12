// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick 2.0

QtObject {
    property int rejectValue: 5
    property var promise: Promise.reject(rejectValue)
    property bool wasTestSuccessful: false

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
