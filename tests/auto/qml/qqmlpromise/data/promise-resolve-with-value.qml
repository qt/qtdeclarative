// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
import QtQuick 2.0

QtObject {
    property int resolveValue: 5
    property var promise: Promise.resolve(resolveValue)
    property bool wasTestSuccessful: false

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
