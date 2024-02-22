// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick 2.0

QtObject {
    property int resolveValue: 5
    property var originalPromise: Promise.resolve(resolveValue)

    property var castPromise: Promise.resolve(originalPromise)
    property bool wasTestSuccessful: false

    Component.onCompleted: {
        if (castPromise !== originalPromise) {
            console.log("resolve did not return original promise")
            return;
        }

        castPromise.then(function(value) {
            if (value !== resolveValue) {
                console.log("resolved values are not the same")
                return;
            }

            wasTestSuccessful = true
        }, function() {
            throw new Error("Should never be called")
        })
    }
}
