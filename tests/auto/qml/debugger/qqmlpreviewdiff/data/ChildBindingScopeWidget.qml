// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml
import QtQuick

Item {
    id: root
    property bool active: true
    property alias button: tmr

    Rectangle {
        id: container
        width: 100
        height: 100

        Rectangle {
            id: inner
            anchors.fill: parent

            Timer {
                id: tmr
                interval: root.active ? 500 : 1000
            }
        }
    }
}
