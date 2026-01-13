// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

DelegateContainer {
    id: root

    implicitWidth: Math.max(delegateInstance ? delegateInstance.implicitWidth : 0, indicatorLayout.implicitWidth)
    implicitHeight: Math.max(delegateInstance ? delegateInstance.implicitHeight : 0, indicatorLayout.implicitHeight)

    transformOrigin: Item.TopLeft
    rotation: vertical ? 90 : 0
    scale: vertical ? -1 : 1
    delegateProperties: root.indicatorProperties

    required property StyleKitDelegateProperties indicatorProperties
    property bool vertical: false
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
                alignment: indicatorProperties.foreground.alignment
                margins.left: indicatorProperties.foreground.leftMargin
                margins.right: indicatorProperties.foreground.rightMargin
                margins.top: indicatorProperties.foreground.topMargin
                margins.bottom: indicatorProperties.foreground.bottomMargin
                fillWidth: indicatorProperties.foreground.implicitWidth === Style.Stretch
                fillHeight: indicatorProperties.foreground.implicitHeight === Style.Stretch
            }
        ]
        mirrored: parentControl.mirrored
    }

    DelegateContainer {
        id: foreground
        parent: root
        parentControl: root.parentControl
        delegateProperties: root.indicatorProperties.foreground
        x: fgItem.x + firstProgress * (fgItem.fillWidth
                ? fgItem.width - indicatorProperties.foreground.minimumWidth
                : fgItem.width)
        y: fgItem.y
        z: 1
        width: fgItem.fillWidth ? (indicatorProperties.foreground.minimumWidth
                                    + ((secondProgress - firstProgress) * (fgItem.width
                                        - indicatorProperties.foreground.minimumWidth)))
                                : (secondProgress - firstProgress) * fgItem.width
        height: fgItem.height
    }
}
