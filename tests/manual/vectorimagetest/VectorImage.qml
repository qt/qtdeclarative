// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.VectorImage
import QtQuick.Controls
import Qt.labs.lottieqt

Item {
    width: vectorImage.visible ? vectorImage.implicitWidth * (VectorImageManager.scale / 10.0) : 500
    height: vectorImage.visible ? vectorImage.implicitHeight * (VectorImageManager.scale / 10.0) : 500
    scale: vectorImage.visible ? (VectorImageManager.scale / 10.0) : 1
    transformOrigin: Item.TopLeft

    Image {
        source: "background.png"
        fillMode: Image.Tile
        horizontalAlignment: Image.AlignLeft
        verticalAlignment: Image.AlignTop
        scale: 1.0 / parent.scale
        width: parent.width
        height: parent.height
        transformOrigin: Item.TopLeft
        visible: vectorImage.visible
    }

    VectorImage {
        id: vectorImage
        source: VectorImageManager.currentSource
        preferredRendererType: VectorImage.CurveRenderer
        assumeTrustedSource: true
        animations.loops: VectorImageManager.looping ? Animation.Infinite : 1
        asynchronous: true
        visible: status === VectorImage.Ready
        animations.paused: !VectorImageManager.playing

        LottieVectorImageController {
            id: controller
            target: VectorImageManager.currentSource.toString().endsWith("json") ? vectorImage : null
            onCurrentFrameChanged: {
                VectorImageManager.currentTime = 100.0 * (controller.currentFrame - controller.startFrame) / (controller.endFrame - controller.startFrame)
            }
        }
    }

    property bool isUpdating: false
    Connections {
        target: VectorImageManager

        function onCurrentTimeChanged(time) {
            if (!VectorImageManager.playing && !isUpdating) {
                isUpdating = true
                var frame = ((time / 100.0) * (controller.endFrame - controller.startFrame) + controller.startFrame)
                controller.gotoAndStop(frame)
                isUpdating = false
            }
        }
    }

    BusyIndicator {
        anchors.centerIn: parent
        visible: running
        running: vectorImage.status === VectorImage.Loading
    }
}
