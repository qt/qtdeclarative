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

    /* Note: Unlike other controls, the GroupBox topPadding includes the
     * background's topMargin, which pushes the frame below the label.
     * This ensures that padding still represents the space between the
     * frame and the child controls, consistent with Pane and Frame. */
    topPadding: styleReader.topPadding + styleReader.background.topMargin

    leftPadding: styleReader.leftPadding
    rightPadding: styleReader.rightPadding
    bottomPadding: styleReader.bottomPadding

    leftInset: styleReader.background.leftMargin
    topInset: styleReader.background.topMargin
    rightInset: styleReader.background.rightMargin
    bottomInset: styleReader.background.bottomMargin

    font: styleReader.font

    StyleVariation.controlType: styleReader.controlType
    StyleReader {
        id: styleReader
        controlType: StyleReader.GroupBox
        enabled: control.enabled
        focused: control.activeFocus
        hovered: control.hovered
        palette: control.palette
    }

    label: Text {
        x: control.leftPadding
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
        backgroundStyle: styleReader.background
    }
}
