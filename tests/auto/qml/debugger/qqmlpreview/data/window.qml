// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QtQuick.Window 2.0

Window {
    visible: true

    height: 100
    width: 100

    Timer {
        repeat: false
        interval: 1000
        running: true
        onTriggered: console.log("window.qml");
    }
}
