// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Image {
    source: "quit.png"
    scale: quitMouse.pressed ? 0.8 : 1.0
    MouseArea {
        id: quitMouse
        anchors.fill: parent
        anchors.margins: -10
        onClicked: Qt.quit()
    }
}
