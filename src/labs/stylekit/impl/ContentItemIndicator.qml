// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

Item {
    id: root

    /* Some controls (e.g. ToolSeparator) have no dedicated indicator property
     * and assign their indicator to contentItem instead. Qt Quick Controls
     * stretches contentItem to fill the control's content area, but StyleKit
     * indicators are always sized by their implicit size (or fillWidth / fillHeight)
     * and positioned by their alignment. This delegate therefore wraps the indicator
     * in an Item that fills the content area, and then uses StyleKitLayout to apply the
     * correct sizing and alignment rules, making the styling API consistent regardless
     * of whether the user styles a 'checkBox.indicator' or a 'toolSeparator.indicator' */

    implicitWidth: indicatorStyle.implicitWidth
    implicitHeight: indicatorStyle.implicitHeight
    visible: indicatorStyle.visible

    required property DelegateStyle indicatorStyle
    required property QtObject quickControl

    StyleKitLayout {
        id: layout
        container: root
        enabled: true
        layoutItems: [
            StyleKitLayoutItem {
                id: layoutItem
                item: indicator
                alignment: indicatorStyle.alignment
                margins.left: indicatorStyle.leftMargin
                margins.right: indicatorStyle.rightMargin
                margins.top: indicatorStyle.topMargin
                margins.bottom: indicatorStyle.bottomMargin
                fillWidth: indicatorStyle.fillWidth
                fillHeight: indicatorStyle.fillHeight
            }
        ]
        mirrored: quickControl.mirrored
    }

    IndicatorDelegate {
        id: indicator
        quickControl: root.quickControl
        indicatorStyle: root.indicatorStyle
        x: layoutItem.x
        y: layoutItem.y
        width: layoutItem.width
        height: layoutItem.height
    }
}
