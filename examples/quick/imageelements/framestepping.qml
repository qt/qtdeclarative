// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

Rectangle {
    width: 480
    height: 320
    Image {
        id: img

        anchors.centerIn: parent
        cache: true
        source: "pics/multi.ico"

        Shortcut {
            sequence: StandardKey.MoveToNextPage
            enabled: img.currentFrame < img.frameCount - 1
            onActivated: img.currentFrame++
        }
        Shortcut {
            sequence: StandardKey.MoveToPreviousPage
            enabled: img.currentFrame > 0
            onActivated: img.currentFrame--
        }
    }

    Label {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.margins: 6
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("frame %1 of %2 \nPress PgUp/PgDn to switch frames")
            .arg(img.currentFrame + 1)
            .arg(img.frameCount)
    }
}
