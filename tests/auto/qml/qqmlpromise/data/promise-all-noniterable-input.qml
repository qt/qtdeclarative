// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
import QtQuick 2.4

QtObject {
    property bool wasTestSuccessful: false

    property var nonIterable: 3
    property var promise: Promise.all(nonIterable);

    Component.onCompleted: {
        promise.then(function() {
            throw new Error("Should never be called")
        }, function(err) {
            if (!(err instanceof TypeError)) {
                throw new Error("Should reject with TypeError")
                return;
            }

            wasTestSuccessful = true
        })
    }
}
