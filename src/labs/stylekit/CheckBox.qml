// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

T.CheckBox {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    leftPadding: checkBoxLayout.padding.left
    topPadding: checkBoxLayout.padding.top
    rightPadding: checkBoxLayout.padding.right
    bottomPadding: checkBoxLayout.padding.bottom

    spacing: styleReader.spacing

    StyleKitControl.controlType: styleReader.type
    StyleKitReader {
        id: styleReader
        type: StyleKitReader.CheckBox
        enabled: control.enabled
        focused: control.activeFocus
        checked: control.checked
        hovered: control.hovered
        pressed: control.pressed
        palette: control.palette
    }

    StyleKitLayout {
        id: checkBoxLayout
        container: control
        contentMargins {
            left: styleReader.leftPadding
            right: styleReader.rightPadding
            top: styleReader.topPadding
            bottom: styleReader.bottomPadding
        }
        layoutItems: [
            // We don't lay out the contentItem here because it occupies the remaining space
            // as calculated by control internal logic.
            StyleKitLayoutItem {
                id: indicatorItem
                item: control.indicator
                alignment: styleReader.indicator.alignment
                margins.left: styleReader.indicator.leftMargin
                margins.right: styleReader.indicator.rightMargin
                margins.top: styleReader.indicator.topMargin
                margins.bottom: styleReader.indicator.bottomMargin
                fillWidth: styleReader.indicator.implicitWidth === Style.Stretch
                fillHeight: styleReader.indicator.implicitHeight === Style.Stretch
            }
        ]
        spacing: styleReader.spacing
        mirrored: control.mirrored
    }

    indicator: IndicatorDelegate {
        parentControl: control
        indicatorProperties: styleReader.indicator
        x: indicatorItem.x
        y: indicatorItem.y
        width: indicatorItem.width
        height: indicatorItem.height
    }

    contentItem: CheckLabel {
        text: control.text
        font: styleReader.font
        color: styleReader.text.color
        horizontalAlignment: styleReader.text.alignment & Qt.AlignHorizontal_Mask
        verticalAlignment: styleReader.text.alignment & Qt.AlignVertical_Mask
        padding: styleReader.text.padding
        topPadding: styleReader.text.topPadding
        bottomPadding: styleReader.text.bottomPadding
        leftPadding: styleReader.text.leftPadding
        rightPadding: styleReader.text.rightPadding
    }

    background: BackgroundDelegate {
        parentControl: control
        backgroundProperties: styleReader.background
    }
}
