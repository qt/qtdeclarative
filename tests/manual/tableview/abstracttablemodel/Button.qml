// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick 2.12

Rectangle {
    width: 110
    height: 40
    border.width: 1
    color: "lightgreen"

    property string text
    signal clicked

    Text {
        anchors.centerIn: parent
        text: parent.text
    }

    MouseArea {
        anchors.fill: parent
        onClicked: parent.clicked()
    }
}
