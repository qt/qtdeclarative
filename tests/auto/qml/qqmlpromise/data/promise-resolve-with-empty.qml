// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick 2.0

QtObject {
    property var promise: Promise.resolve()
    property bool wasTestSuccessful: false

    Component.onCompleted: {
        promise.then(function(value) {
            if (typeof value === "undefined") {
                wasTestSuccessful = true
            }
        }, function() {
            throw new Error("Should never be called")
        })
    }
}
