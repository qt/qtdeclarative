// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

T.HeaderViewDelegate {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    highlighted: control.selected

    leftPadding: styleReader.leftPadding
    rightPadding: styleReader.rightPadding
    topPadding: styleReader.topPadding
    bottomPadding: styleReader.bottomPadding

    leftInset: styleReader.background.leftMargin
    rightInset: styleReader.background.rightMargin
    topInset: styleReader.background.topMargin
    bottomInset: styleReader.background.bottomMargin

    spacing: styleReader.spacing
    font: styleReader.font

    StyleVariation.controlType: styleReader.controlType
    StyleReader {
        id: styleReader
        controlType: StyleReader.VerticalHeaderViewDelegate
        enabled: control.enabled
        checked: control.checked
        hovered: control.hovered
        pressed: control.pressed
        highlighted: control.highlighted
        palette: control.palette
    }

    contentItem: Label {
        color: styleReader.text.color
        text: control.model[control.headerView.textRole]
        font: styleReader.font

        horizontalAlignment: styleReader.text.alignment & Qt.AlignHorizontal_Mask
        verticalAlignment: styleReader.text.alignment & Qt.AlignVertical_Mask
        padding: styleReader.text.padding
        leftPadding: styleReader.text.leftPadding
        rightPadding: styleReader.text.rightPadding
        bottomPadding: styleReader.text.bottomPadding
        topPadding: styleReader.text.topPadding
    }

    background: BackgroundDelegate {
        quickControl: control
        backgroundStyle: styleReader.background
    }
}
