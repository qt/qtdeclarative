// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

T.SwitchDelegate {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    leftPadding: switchDelegateLayout.padding.left
    rightPadding: switchDelegateLayout.padding.right
    topPadding: switchDelegateLayout.padding.top
    bottomPadding: switchDelegateLayout.padding.bottom

    leftInset: styleReader.background.leftMargin
    rightInset: styleReader.background.rightMargin
    topInset: styleReader.background.topMargin
    bottomInset: styleReader.background.bottomMargin

    spacing: styleReader.spacing

    icon.width: 16
    icon.height: 16
    icon.color: styleReader.text.color

    font: styleReader.font

    StyleVariation.controlType: styleReader.controlType
    StyleReader {
        id: styleReader
        controlType: StyleReader.SwitchDelegate
        enabled: control.enabled
        focused: control.activeFocus
        checked: control.checked
        hovered: control.hovered
        pressed: control.pressed
        highlighted: control.highlighted
        palette: control.palette
    }

    StyleKitLayout {
        id: switchDelegateLayout
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
                fillWidth: styleReader.indicator.fillWidth
                fillHeight: styleReader.indicator.fillHeight
            }
        ]
        spacing: styleReader.spacing
        mirrored: control.mirrored
    }

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        icon: control.icon
        text: control.text
        font: control.font
        color: styleReader.text.color
        alignment: styleReader.text.alignment
        topPadding: styleReader.text.topPadding
        bottomPadding: styleReader.text.bottomPadding
        leftPadding: styleReader.text.leftPadding
        rightPadding: styleReader.text.rightPadding
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

    background: BackgroundDelegate {
        quickControl: control
        backgroundStyle: styleReader.background
    }
}
