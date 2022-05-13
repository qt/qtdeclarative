// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
import QtQuick 2.4

QtObject {
    property int resolveValue: 5
    property var resultValue: [resolveValue, resolveValue, resolveValue]
    property bool wasTestSuccessful: false


    property var delayedEvent: Timer {
        interval: 0
        property var handler: null
        onTriggered: {
            if (handler) {
                handler();
            }
        }
    }

    function postEvent(event, value) {
        delayedEvent.handler = function() { event(value) }
        delayedEvent.restart();
    }

    property var promise1: Promise.resolve(resolveValue);
    property int promise2: resolveValue
    property var promise3: new Promise(function (resolve, reject) {
        postEvent(resolve, resolveValue)
    })

    Component.onCompleted: {
        Promise.all([promise1, promise2, promise3]).then(function(value) {
            if (value.length !== resultValue.length) {
                return;
            }

            for (var i in value) {
                if (value[i] !== resultValue[i]) {
                    return;
                }
            }

            wasTestSuccessful = true

        }, function() {
            throw new Error("Should never be called")
        })
    }
}
