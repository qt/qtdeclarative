// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Effects

Item {
    id: rootItem
    // We expect all effects to be placed under ItemSwitcher
    property Item switcher: rootItem.parent

    property real _xPos: Math.sin(switcher.inAnimation * Math.PI * 50) * width * 0.03 * (0.5 - Math.abs(0.5 - switcher.inAnimation))
    property real _yPos: Math.sin(switcher.inAnimation * Math.PI * 35) * width * 0.02 * (0.5 - Math.abs(0.5 - switcher.inAnimation))

    anchors.fill: parent

    Image {
        id: maskImage
        source: "images/stripes.png"
        visible: false
    }

    MultiEffect {
        source: switcher.fromItem
        width: parent.width
        height: parent.height
        x: rootItem._xPos
        y: rootItem._yPos
        blurEnabled: true
        blur: switcher.inAnimation
        blurMax: 32
        blurMultiplier: 0.5
        opacity: switcher.outAnimation
        colorizationColor: "#f0d060"
        colorization: switcher.inAnimation

        contrast: switcher.inAnimation
        brightness: switcher.inAnimation

        maskEnabled: true
        maskSource: maskImage
        maskThresholdMin: switcher.inAnimation * 0.9
        maskSpreadAtMin: 0.2
        maskThresholdMax: 1.0

        shadowEnabled: true
        shadowColor: "#f04000"
        shadowBlur: 1.0
        shadowOpacity: 5.0 - switcher.outAnimation * 5.0
        shadowHorizontalOffset: 0
        shadowVerticalOffset: 0
        shadowScale: 1.04

    }
    MultiEffect {
        source: switcher.toItem
        width: parent.width
        height: parent.height
        x: -rootItem._xPos
        y: -rootItem._yPos
        blurEnabled: true
        blur: switcher.outAnimation * 2
        blurMax: 32
        blurMultiplier: 0.5
        opacity: switcher.inAnimation * 3.0 - 1.0

        colorizationColor: "#f0d060"
        colorization: switcher.outAnimation
        contrast: switcher.outAnimation
        brightness: switcher.outAnimation

        maskEnabled: true
        maskSource: maskImage
        maskThresholdMin: switcher.outAnimation * 0.6
        maskSpreadAtMin: 0.2
        maskThresholdMax: 1.0

        shadowEnabled: true
        shadowColor: "#f04000"
        shadowBlur: 1.0
        shadowOpacity: 5.0 - switcher.inAnimation * 5.0
        shadowHorizontalOffset: 0
        shadowVerticalOffset: 0
        shadowScale: 1.04
    }

}
