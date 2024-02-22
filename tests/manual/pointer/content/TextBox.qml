// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.12

Rectangle {
    id: r1
    radius: 10
    border.color: "black"
    border.width: 4
    property string label
    implicitHeight: txt.implicitHeight + 12
    implicitWidth: txt.implicitWidth+ 12
    color: "#c0c0c0"

    function queryColor(pressed) {
        return pressed ? "#ff4040" : "#c0c0c0"
    }

    Text {
        id: txt
        y: 6
        anchors.horizontalCenter: parent.horizontalCenter
        text: parent.label
    }
}
