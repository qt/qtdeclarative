// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

//DO NOT CHANGE
Item {
    Timer {
        id: timer;  interval: 1; running: true; repeat: true; triggeredOnStart:  true
        onTriggered: {
            console.log("timer");
        }
    }
}
