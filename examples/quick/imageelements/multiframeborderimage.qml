// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

Rectangle {
    width: 480
    height: 320
    BorderImage {
        id: img

        anchors.fill: parent
        anchors.margins: 6
        cache: true
        source: "pics/multi.ico"
        border {
            left: 19
            top: 19
            right: 19
            bottom: 19
        }
        horizontalTileMode: BorderImage.Stretch

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
        anchors.centerIn: parent
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("frame " + (img.currentFrame + 1) + " of " + img.frameCount +
              "\nPress PgUp/PgDn to switch frames")
    }
}
