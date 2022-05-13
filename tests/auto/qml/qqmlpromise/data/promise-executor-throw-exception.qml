// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
import QtQuick 2.0

QtObject {
    property bool wasTestSuccessful: false

    property var errorObject: { text: "Exception should not escape executor" }

    property var promise: new Promise(function() {
        throw errorObject
    })

    Component.onCompleted: {
        promise.then(function() {
            throw new Error("Should never be called")
        }, function(error) {
            if (error === errorObject) {
                wasTestSuccessful = true
            }
        })
    }
}
