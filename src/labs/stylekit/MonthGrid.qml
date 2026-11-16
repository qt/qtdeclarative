// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

T.AbstractMonthGrid {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                        implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                            implicitContentHeight + topPadding + bottomPadding)

    rightPadding: styleReader.rightPadding
    leftPadding: styleReader.leftPadding
    topPadding: styleReader.topPadding
    bottomPadding: styleReader.bottomPadding

    rightInset: styleReader.background.rightMargin
    leftInset: styleReader.background.leftMargin
    topInset: styleReader.background.topMargin
    bottomInset: styleReader.background.bottomMargin

    spacing: styleReader.spacing
    font: styleReader.font

    StyleVariation.controlType: styleReader.controlType
    StyleReader {
        id: styleReader
        controlType: StyleReader.MonthGrid
        enabled: control.enabled
        focused: control.activeFocus
        hovered: control.hovered
        palette: control.palette
    }

    delegate: Text {
        opacity: model.month === control.month ? 1 : 0
        text: model.day
        font: control.font
        color: styleReader.text.color

        padding: styleReader.text.padding
        leftPadding: styleReader.text.leftPadding
        rightPadding: styleReader.text.rightPadding
        bottomPadding: styleReader.text.bottomPadding
        topPadding: styleReader.text.topPadding
        verticalAlignment: styleReader.text.alignment
        horizontalAlignment: styleReader.text.alignment

        required property var model
    }

    contentItem: Grid {
        rows: 6
        columns: 7
        rowSpacing: control.spacing
        columnSpacing: control.spacing

        Repeater {
            model: control.source
            delegate: control.delegate
        }
    }

    background: BackgroundDelegate {
        quickControl: control
        backgroundStyle: styleReader.background
    }
}
