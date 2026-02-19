// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

T.Switch {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    leftPadding: switchLayout.padding.left
    topPadding: switchLayout.padding.top
    rightPadding: switchLayout.padding.right
    bottomPadding: switchLayout.padding.bottom

    leftInset: styleReader.background.leftMargin
    topInset: styleReader.background.topMargin
    rightInset: styleReader.background.rightMargin
    bottomInset: styleReader.background.bottomMargin

    spacing: styleReader.spacing

    font: styleReader.font

    StyleVariation.controlType: styleReader.controlType
    StyleReader {
        id: styleReader
        controlType: StyleReader.SwitchControl
        enabled: control.enabled
        focused: control.activeFocus
        hovered: control.hovered
        pressed: control.pressed
        checked: control.checked
        palette: control.palette
    }

    StyleKitLayout {
        id: switchLayout
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
        quickControl: control
        indicatorStyle: styleReader.indicator
        x: indicatorItem.x
        y: indicatorItem.y
        width: indicatorItem.width
        height: indicatorItem.height

        HandleDelegate {
            quickControl: control
            handleStyle: styleReader.handle
            x: control.checked
               ? indicator.width - width - styleReader.handle.rightMargin
               : styleReader.handle.leftMargin
            y: styleReader.handle.topMargin - styleReader.handle.bottomMargin
                + (indicator.height - height) / 2
            z: 1
            Behavior on x { NumberAnimation { duration: 50 } } // factor animation out to a style property!
        }
    }

    contentItem: CheckLabel {
        text: control.text
        font: control.font
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
        quickControl: control
        backgroundStyle: styleReader.background
    }
}
