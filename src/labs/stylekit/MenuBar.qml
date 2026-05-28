// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

T.MenuBar {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    topPadding: SafeArea.margins.top + styleReader.topPadding
    leftPadding: SafeArea.margins.left + styleReader.leftPadding
    rightPadding: SafeArea.margins.right + styleReader.rightPadding
    bottomPadding: SafeArea.margins.bottom + styleReader.bottomPadding

    leftInset: styleReader.background.leftMargin
    topInset: styleReader.background.topMargin
    rightInset: styleReader.background.rightMargin
    bottomInset: styleReader.background.bottomMargin

    spacing: styleReader.spacing

    font: styleReader.font

    StyleVariation.controlType: styleReader.controlType
    StyleReader {
        id: styleReader
        controlType: StyleReader.MenuBar
        enabled: control.enabled
        focused: control.activeFocus
        hovered: control.hovered
        palette: control.palette
    }

    delegate: MenuBarItem { }

    contentItem: Row {
        spacing: control.spacing
        Repeater {
            model: control.contentModel
        }
    }

    background: BackgroundDelegate {
        quickControl: control
        backgroundStyle: styleReader.background
    }
}
