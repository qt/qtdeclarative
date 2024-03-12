// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtQuick.Window 2.3

Window {
    visible: true

    height: 200
    width: 100

    Timer {
        interval: 1000
        running: true
        onTriggered: console.log("window1.qml");
    }
}
