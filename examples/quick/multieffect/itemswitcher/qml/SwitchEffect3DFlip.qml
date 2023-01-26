// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Effects

Item {
    id: rootItem
    // We expect all effects to be placed under ItemSwitcher
    property Item switcher: rootItem.parent

    anchors.fill: parent

    MultiEffect {
        source: switcher.fromItem
        width: parent.width
        height: parent.height
        x: switcher.inAnimation * rootItem.width
        blurEnabled: true
        blur: switcher.inAnimation
        blurMax: 32
        blurMultiplier: 0.5
        opacity: switcher.outAnimation

        saturation: -switcher.inAnimation * 1.5

        maskEnabled: true
        maskSource: Image {
            source: "images/smoke.png"
            visible: false
        }
        maskThresholdMin: switcher.inAnimation * 0.6
        maskSpreadAtMin: 0.1
        maskThresholdMax: 1.0 - (switcher.inAnimation * 0.6)
        maskSpreadAtMax: 0.1

        shadowEnabled: true
        shadowOpacity: 0.5
        shadowBlur: 0.8
        shadowVerticalOffset: 5
        shadowHorizontalOffset: 10 + (x * 0.2)
        shadowScale: 1.02

        transform: Rotation {
            origin.x: parent.width / 2
            origin.y: parent.height / 2
            axis { x: 0; y: 1; z: 0 }
            angle: switcher.inAnimation * 60
        }
        rotation: -switcher.inAnimation * 20
        scale: 1.0 + (switcher.inAnimation * 0.2)
    }

    MultiEffect {
        source: switcher.toItem
        width: parent.width
        height: parent.height
        x: -switcher.outAnimation * rootItem.width
        blurEnabled: true
        blur: switcher.outAnimation * 2
        blurMax: 32
        blurMultiplier: 0.5
        opacity: switcher.inAnimation

        saturation: -switcher.outAnimation * 1.5

        maskEnabled: true
        maskSource: Image {
            source: "images/smoke.png"
            visible: false
        }
        maskThresholdMin: switcher.outAnimation * 0.6
        maskSpreadAtMin: 0.1
        maskThresholdMax: 1.0 - (switcher.outAnimation * 0.6)
        maskSpreadAtMax: 0.1

        shadowEnabled: true
        shadowOpacity: 0.5
        shadowBlur: 0.8
        shadowVerticalOffset: 5
        shadowHorizontalOffset: 10 + (x * 0.2)
        shadowScale: 1.02

        transform: Rotation {
            origin.x: parent.width / 2
            origin.y: parent.height / 2
            axis { x: 0; y: 1; z: 0 }
            angle: -switcher.outAnimation * 60
        }
        rotation: switcher.outAnimation * 20
        scale: 1.0 - (switcher.outAnimation * 0.4)
    }
}
