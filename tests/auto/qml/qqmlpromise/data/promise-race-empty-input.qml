// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
import QtQuick 2.4

QtObject {
    property bool wasTestSuccessful: true

    Component.onCompleted: {
        Promise.race([]).then(function(value) {
            wasTestSuccessful = false
            throw new Error("Should never be called")
        }, function() {
            throw new Error("Should never be called")
        })
    }
}
