// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

Item {
    id: root

    implicitWidth: backgroundStyle.width
    implicitHeight: backgroundStyle.height
    visible: backgroundStyle.visible

    required property DelegateStyle backgroundStyle
    required property QtObject quickControl

    StyleKitLayout {
        id: layout
        container: root
        enabled: true
        layoutItems: [
            StyleKitLayoutItem {
                id: layoutItem
                item: background
                alignment: backgroundStyle.alignment
                fillWidth: backgroundStyle.fillWidth
                fillHeight: backgroundStyle.fillHeight
            }
        ]
        mirrored: !!quickControl.mirrored
    }

    DelegateContainer {
        id: background
        quickControl: root.quickControl
        delegateStyle: root.backgroundStyle
        x: layoutItem.x
        y: layoutItem.y
        width: layoutItem.width
        height: layoutItem.height
    }
}
