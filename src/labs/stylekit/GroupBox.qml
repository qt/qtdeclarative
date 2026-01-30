// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default
import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

T.GroupBox {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding,
                            implicitLabelWidth)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    spacing: styleReader.spacing

    topPadding: styleReader.topPadding + implicitLabelWidth > 0 ? implicitLabelHeight + spacing : 0
    leftPadding: styleReader.leftPadding
    rightPadding: styleReader.rightPadding
    bottomPadding: styleReader.bottomPadding

    topInset: styleReader.background.topMargin
    bottomInset: styleReader.background.bottomMargin
    rightInset: styleReader.background.rightMargin
    leftInset: styleReader.background.leftMargin

    font: styleReader.font

    StyleKitControl.controlType: styleReader.type
    StyleKitReader {
        id: styleReader
        type: StyleKitReader.GroupBox
        enabled: control.enabled
        focused: control.activeFocus
        hovered: control.hovered
        palette: control.palette
    }

    label: Text {
        padding: styleReader.text.padding
        leftPadding: styleReader.text.leftPadding
        rightPadding: styleReader.text.rightPadding
        bottomPadding: styleReader.text.bottomPadding
        topPadding: styleReader.text.topPadding
        verticalAlignment: styleReader.text.alignment
        horizontalAlignment: styleReader.text.alignment
        color: styleReader.text.color
        font: control.font
        width: control.width
        text: control.title
        elide: Text.ElideRight
    }

    background: BackgroundDelegate {
        quickControl: control
        backgroundProperties: styleReader.background
    }
}
