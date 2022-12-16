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
        anchors.fill: parent
        blurEnabled: true
        blur: switcher.inAnimation * 4
        blurMax: 32
        blurMultiplier: 0.5
        opacity: switcher.outAnimation
        saturation: -switcher.inAnimation * 2
    }
    MultiEffect {
        source: switcher.toItem
        anchors.fill: parent
        blurEnabled: true
        blur: switcher.outAnimation * 4
        blurMax: 32
        blurMultiplier: 0.5
        opacity: switcher.inAnimation
        saturation: -switcher.outAnimation * 2
    }
}
