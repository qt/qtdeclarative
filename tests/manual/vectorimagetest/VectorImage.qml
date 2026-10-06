// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.VectorImage
import QtQuick.Controls
import Qt.labs.lottieqt

Item {
    property bool isLottie: VectorImageManager.currentSource.toString().endsWith("json")
    property alias isNonDefault: vectorImage._qt_usenondefaultgenerator
    property int vectorImageWidth: vectorImage.visible ? vectorImage.implicitWidth : 500
    property int vectorImageHeight: vectorImage.visible ? vectorImage.implicitHeight : 500

    width: vectorImageWidth * scale
    height: vectorImageHeight * scale
    scale: VectorImageManager.scale / 10.0
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
    }

    VectorImage {
        id: vectorImage
        property bool _qt_usenondefaultgenerator: false

        source: VectorImageManager.currentSource
        preferredRendererType: VectorImage.CurveRenderer
        assumeTrustedSource: true
        animations.loops: VectorImageManager.looping ? Animation.Infinite : 1
        asynchronous: true
        visible: status === VectorImage.Ready
        animations.paused: !VectorImageManager.playing || (isLottie && isNonDefault)

        LottieVectorImageController {
            id: controller
            target: isLottie ? vectorImage : null
            onCurrentFrameChanged: {
                if (!isNonDefault)
                    VectorImageManager.currentTime = 100.0 * (controller.currentFrame - controller.startFrame) / (controller.endFrame - controller.startFrame)
            }
        }
    }

    property bool isUpdating: false
    Connections {
        target: VectorImageManager

        function onCurrentTimeChanged(time) {
            if (!isUpdating && (!VectorImageManager.playing || (isLottie && isNonDefault))) {
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
