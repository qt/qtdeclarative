// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtGraphicalEffects
import QtQuick

Item {
    id: root
    width: PathView.view.delegateSize
    height: width
    z: PathView.zOrder

    transform: [
        Rotation {
            angle: root.PathView.rotateY
            origin.x: sourceImage.width / 2
            origin.y: sourceImage.height * 0.3
            axis.x: 0
            axis.y: 1
            axis.z: 0
        },
        Scale {
            xScale: 1.0
            yScale: root.PathView.scale
            origin.x: sourceImage.width / 2
            origin.y: sourceImage.height * 0.4
        }
    ]

    Image {
        id: sourceImage
        width: root.PathView.view.delegateSize
        height: width
        fillMode: Image.PreserveAspectFit
        source: "file:/" + docImagesDir + model.source

        Rectangle {
            x: (sourceImage.width - sourceImage.paintedWidth) / 2
            width: sourceImage.paintedWidth + (index == 6 ? 2 : 1)
            height: sourceImage.height
            color: "transparent"
            border.color: "#f4f4f4"
            antialiasing: true
            visible: !model.dark
        }
    }

    ShaderEffectSource {
        id: reflection
        sourceItem: sourceImage
        y: sourceItem.height
        width: sourceItem.width
        height: sourceItem.height

        transform: [
            Rotation {
                origin.x: reflection.width / 2
                origin.y: reflection.height / 2
                axis.x: 1
                axis.y: 0
                axis.z: 0
                angle: 180
            }
        ]
    }

    Rectangle {
        objectName: sourceImage.source.toString().slice(-20)
        x: (parent.width - sourceImage.paintedWidth) / 2// + (paintedWidthDiff > 0 ? 1.0 - paintedWidthDiff : 0)
        y: reflection.y
        width: sourceImage.paintedWidth + (index == 6 ? 2 : 1)
        height: sourceImage.paintedHeight

        // TODO: figure out how to get perfect x/width without using the current width hack
//        readonly property real paintedWidthDiff: sourceImage.paintedWidth - Math.floor(sourceImage.paintedWidth)

        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: Qt.rgba(backgroundColor.r, backgroundColor.g, backgroundColor.b, 0.33)
            }
            GradientStop {
                // This determines the point at which the reflection fades out.
                position: 1.0
                color: backgroundColor
            }
        }
    }
}
