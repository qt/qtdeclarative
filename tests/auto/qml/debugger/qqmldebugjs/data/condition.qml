// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

//DO NOT CHANGE

Item {
    id: item
    property int a: 0
    Timer {
        id: timer;  interval: 1; repeat: true; running: true
        onTriggered: a++
    }
}

