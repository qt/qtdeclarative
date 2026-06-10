// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

DelegateContainer {
    id: root

    implicitWidth: Math.max(delegateInstance ? delegateInstance.implicitWidth : 0, indicatorLayout.implicitWidth)
    implicitHeight: Math.max(delegateInstance ? delegateInstance.implicitHeight : 0, indicatorLayout.implicitHeight)

    delegateStyle: root.indicatorStyle
    visible: delegateStyle.visible
    required property DelegateStyle indicatorStyle

    /* Some indicators (Slider, RangeSlider) should let the foreground delegate
     * only fill up a certain amount of the available foreground space (that is, the
     * track / progress). This amount be controlled with firstProgress and secondProgress. */
    property real firstProgress: 0.0
    property real secondProgress: 1.0

    StyleKitLayout {
        id: indicatorLayout
        container: root
        enabled: true
        layoutItems: [
            StyleKitLayoutItem {
                id: fgItem
                item: foreground
                alignment: indicatorStyle.foreground.alignment
                margins.left: indicatorStyle.foreground.leftMargin
                margins.right: indicatorStyle.foreground.rightMargin
                margins.top: indicatorStyle.foreground.topMargin
                margins.bottom: indicatorStyle.foreground.bottomMargin
                fillWidth: indicatorStyle.foreground.fillWidth
                fillHeight: indicatorStyle.foreground.fillHeight
            }
        ]
        mirrored: quickControl.mirrored
    }

    DelegateContainer {
        id: foreground
        parent: root
        quickControl: root.quickControl
        delegateStyle: root.indicatorStyle.foreground
        visible: root.indicatorStyle.foreground.visible
        x: fgItem.x
        y: fgItem.y
        z: 1
        width: fgItem.width
        height: fgItem.height
    }
}
