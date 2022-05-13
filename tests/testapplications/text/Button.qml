// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

MouseArea {
    property string buttontext: ""
    width: parent.width / 4
    height: parent.height - 4
    Rectangle {
        anchors.fill: parent
        radius: 5
        color: "lightgray"
        border.color: "black"
    }
    Text {
        anchors.centerIn: parent
        text: buttontext
        color: "black"
    }
}
