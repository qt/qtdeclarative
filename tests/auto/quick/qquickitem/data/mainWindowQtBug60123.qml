// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Rectangle {
    width: 400
    height: 200
    color: "white"
    property string lastEvent: ""

    Rectangle {
        id: buttonRect
        width: 50
        height: 50
        color: "black"

        MouseArea {
            anchors.fill: parent
            onPressed: {
                lastEvent = "pressed"
                buttonRect.color = "yellow"
            }
            onReleased: {
                lastEvent = "released"
                buttonRect.color = "black"
            }
            onCanceled: {
                lastEvent = "canceled"
                buttonRect.color = "green"
            }
        }
    }
}
