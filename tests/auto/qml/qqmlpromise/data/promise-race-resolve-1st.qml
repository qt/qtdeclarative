// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick 2.4

QtObject {
    property int resolveValue: 33
    property var resolvedPromiseArray: [Promise.resolve(resolveValue), Promise.resolve(resolveValue + 5)]
    property bool wasTestSuccessful: false

    Component.onCompleted: {
        Promise.race(resolvedPromiseArray).then(function(value) {
            if (value !== resolveValue) {
                return;
            }

            wasTestSuccessful = true
        }, function() {
            throw new Error("Should never be called")
        })
    }
}
