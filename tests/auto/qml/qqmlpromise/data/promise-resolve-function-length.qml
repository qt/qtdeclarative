// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick 2.0

QtObject {
    property var resolveFunction
    property var promise: new Promise(function(resolve, reject) {
                             resolveFunction = resolve
                          })

    property bool wasTestSuccessful: (typeof resolveFunction === "function" &&
                                      typeof resolveFunction.length !== "undefined" &&
                                      resolveFunction.length === 1)

    Component.onCompleted: {
        // TODO: Function length field should be NotWritabel & NotEnumerable & Configurable
        console.log(Object.getOwnPropertyDescriptor(resolveFunction, "length").configurable)
    }
}
