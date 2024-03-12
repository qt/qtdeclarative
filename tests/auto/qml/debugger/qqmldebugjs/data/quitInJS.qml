// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import "quit.js" as Quit;

//DO NOT CHANGE

Item {
    Timer {
        running: true
        triggeredOnStart: true
        onTriggered: Quit.quit();
    }
}

