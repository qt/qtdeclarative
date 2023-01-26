// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Effects

Item {
    id: rootItem
    // We expect all effects to be placed under ItemSwitcher
    property Item switcher: rootItem.parent
    property real rotation: 0

    anchors.fill: parent

    Item {
        id: mask
        anchors.fill: parent
        layer.enabled: true
        visible: false
        smooth: false
        clip: true
        Image {
            anchors.fill: parent
            anchors.margins: -parent.width * 0.25
            source: "images/hblinds.png"
            rotation: rootItem.rotation
            smooth: false
        }
    }

    // Item going out
    MultiEffect {
        source: switcher.fromItem
        anchors.fill: parent
        maskEnabled: true
        maskSource: mask
        maskThresholdMin: switcher.inAnimation
        maskSpreadAtMin: 0.4
    }
    // Item coming in
    MultiEffect {
        source: switcher.toItem
        anchors.fill: parent
        maskEnabled: true
        maskSource: mask
        maskThresholdMax: switcher.inAnimation
        maskSpreadAtMax: 0.4
    }
}
