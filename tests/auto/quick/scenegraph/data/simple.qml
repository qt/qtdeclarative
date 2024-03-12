// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.2

Rectangle {
    width: 200
    height: 200
    color: "steelblue"
    Rectangle {
        width: 150
        height: 150
        anchors.centerIn: parent
        color: "palegreen"
        rotation: 45
        Text {
            rotation: -45
            text: "Simple QML.."
            anchors.centerIn: parent
        }
    }
}
