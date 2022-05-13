// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.14

Rectangle {
    width: 320; height: 240
    color: "green"; antialiasing: true

    Rectangle {
        width: 100; height: 2; anchors.centerIn: parent
        Rectangle {
            width: 2; height: 100; anchors.centerIn: parent
        }
    }

    WheelHandler {
        activeTimeout: 0.5
    }
}
