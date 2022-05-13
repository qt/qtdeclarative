// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQml 2.0

QtObject {
    property var timer: Timer {
        running: true
        interval: 1
        onTriggered: {
            console.profile();
            stopTimer.start();
        }
    }

    property var stopTimer: Timer {
        id: stopTimer
        interval: 1000
        onTriggered: {
            console.profileEnd();
        }
    }
}
