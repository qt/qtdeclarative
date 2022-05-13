// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
import QtQuick 2.0

QtObject {
    property string resolution: "reject promise";

    property bool wasExecutorCalled: false
    property bool wasPromiseRejected: false
    property bool wasPromiseTypeReturnedByThen: false;
    property bool wasResolutionForwardedCorrectly: false;

    Component.onCompleted: {
        var promise = new Promise(function(resolve, reject) {
            wasExecutorCalled = true;
            reject(resolution);
        });

        var res = promise.then(function(result) {
            wasPromiseRejected = false;
        }, function(err) {
            wasPromiseRejected = true;
            wasResolutionForwardedCorrectly = (err === resolution);
        });

        wasPromiseTypeReturnedByThen = (typeof res === 'Promise');
    }
}
