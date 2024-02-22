// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtMultimedia

Rectangle {
    id: rootItem
    width: 500; height: 500

    // Show the difference in color space for the two windows
    color: "#e0a32e"

    WindowContainer {
        window: Window {
            color: rootItem.color

            VideoOutput {
                id: videoOutput
                anchors.fill: parent
            }

            // Trigger Qt Quick to use a HDR-enabled swap chain
            property var _qt_sg_hdr_format: "scrgb"
        }

        anchors {
            fill: parent
            topMargin: 20
            bottomMargin: 20
        }
    }

    MediaPlayer {
        id: mediaPlayer
        videoOutput: videoOutput
        source: "hdrtest.mp4"
        loops: MediaPlayer.Infinite
        autoPlay: true
    }
}
