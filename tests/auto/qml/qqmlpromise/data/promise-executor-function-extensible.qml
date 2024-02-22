// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick 2.0

QtObject {
    property bool wasTestSuccessful: false

    property var executorFunction: null

    function notPromise(executor) {
        executorFunction = executor;
        executor(function() {}, function() {});
    }

    Component.onCompleted: {
        Promise.resolve.call(notPromise);
        wasTestSuccessful = executorFunction !== null &&
                Object.isExtensible(executorFunction);
    }
}
