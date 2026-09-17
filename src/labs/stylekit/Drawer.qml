// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

T.Drawer {
    id: control

    parent: T.Overlay.overlay

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    leftPadding: styleReader.leftPadding + SafeArea.margins.left + (control.edge === Qt.RightEdge)
    rightPadding: styleReader.rightPadding + SafeArea.margins.right + (control.edge === Qt.LeftEdge)
    topPadding: styleReader.topPadding + SafeArea.margins.top + (control.edge === Qt.BottomEdge)
    bottomPadding: styleReader.bottomPadding + SafeArea.margins.bottom + (control.edge === Qt.TopEdge)

    leftInset: styleReader.background.leftMargin
    topInset: styleReader.background.topMargin
    rightInset: styleReader.background.rightMargin
    bottomInset: styleReader.background.bottomMargin

    spacing: styleReader.spacing

    font: styleReader.font

    StyleVariation.controlType: styleReader.controlType
    StyleReader {
        id: styleReader
        controlType: StyleReader.Drawer
        enabled: control.enabled
        focused: control.activeFocus
        palette: control.palette
    }

    enter: Transition {
        SmoothedAnimation {
            velocity: 5
        }
    }

    exit: Transition {
        SmoothedAnimation {
            velocity: 5
        }
    }

    background: BackgroundDelegate {
        quickControl: control
        backgroundStyle: styleReader.background
    }
}
