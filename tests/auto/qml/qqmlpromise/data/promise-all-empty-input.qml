// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick 2.4

QtObject {
    property bool wasTestSuccessful: false

    property var promise: Promise.all([]);

    Component.onCompleted: {
        promise.then(function(value) {
            if (value.length !== 0) {
                return;
            }

            wasTestSuccessful = true

        }, function() {
            throw new Error("Should never be called")
        })
    }
}
