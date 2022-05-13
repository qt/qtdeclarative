// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
import QtQuick 2.0

QtObject {
    property string resolution: "fullfill promise";

    property bool wasExecutorCalled: false
    property bool wasPromiseResolved: false
    property bool wasPromiseTypeReturnedByThen: false
    property bool wasResolutionForwardedCorrectly: false
    property bool wasNewPromiseObjectCreatedByThen: false

    Component.onCompleted: {
        var promise = new Promise(function(resolve, reject) {
            wasExecutorCalled = true;
            resolve(resolution);
        });

        var res = promise.then(function(result) {
            wasPromiseResolved = true;
            wasResolutionForwardedCorrectly = (result === resolution);
        }, function(err) {
            wasPromiseResolved = false;
        });

        wasPromiseTypeReturnedByThen = (typeof res === 'Promise');
        console.debug("typeof res: " + (typeof res)) // TODO: remove
        wasNewPromiseObjectCreatedByThen = (res !== promise);
    }
}
