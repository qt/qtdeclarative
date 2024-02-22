// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Item {
    width: 400; height: 400

    property alias animRunning: springAnim.running

    Rectangle {
        objectName: "myRect"
        color: "green"
        width: 20; height: 20

        property bool triggered: false

        onXChanged: {
            if (!triggered && x > 50 && x < 80) {
                triggered = true
                x = x //set same value
            }
        }

        Behavior on x {
            SpringAnimation {
                id: springAnim
                spring: 3
                damping: 0.2
                mass: .5
            }
        }
    }
}
