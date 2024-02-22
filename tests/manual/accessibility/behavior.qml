// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {
    id: scene
        width: 400; height: 400

    Rectangle {
        id: rect
        y : 100
        width: 100; height: 100
        color: "red"

        Text {
            text : "Behavior animation"
        }

        Behavior on x {
            NumberAnimation { duration: 1000 }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: rect.x += 50
        }
    }
 }
