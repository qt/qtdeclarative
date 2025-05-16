// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl

T.MenuBar {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    spacing: __config.spacing || 0

    topPadding: __config.topPadding || 0
    bottomPadding: __config.bottomPadding || 0
    leftPadding: __config.leftPadding || 0
    rightPadding: __config.rightPadding || 0

    readonly property var __config: Config.controls.toolbar["normal"] || {}

    delegate: MenuBarItem { }

    contentItem: Row {
        spacing: control.spacing
        Repeater {
            model: control.contentModel
        }
    }

    background: StyleImage {
        imageConfig: control.__config.background
    }
}
