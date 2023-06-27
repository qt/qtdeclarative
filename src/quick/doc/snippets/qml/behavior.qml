// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
import QtQuick

Rectangle {
    id: rect
    width: 100; height: 100
    color: "red"

    Behavior on width {
        NumberAnimation { duration: 1000 }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: rect.width = 50
    }
}
//![0]
