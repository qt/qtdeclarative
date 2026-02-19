// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

T.SpinBox {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    leftPadding: spinBoxLayout.padding.left
    topPadding: spinBoxLayout.padding.top
    rightPadding: spinBoxLayout.padding.right
    bottomPadding: spinBoxLayout.padding.bottom

    leftInset: styleReader.background.leftMargin
    topInset: styleReader.background.topMargin
    rightInset: styleReader.background.rightMargin
    bottomInset: styleReader.background.bottomMargin

    spacing: styleReader.spacing

    font: styleReader.font

    StyleVariation.controlType: styleReader.controlType
    StyleReader {
        id: styleReader
        controlType: StyleReader.SpinBox
        enabled: control.enabled
        focused: control.activeFocus
        hovered: control.hovered || control.down.hovered || control.up.hovered
        pressed: control.down.pressed || control.up.pressed
        palette: control.palette
    }

    StyleReader {
        id: upProperties
        controlType: StyleReader.SpinBox
        enabled: control.enabled
        focused: control.activeFocus
        hovered: control.up.hovered
        pressed: control.up.pressed
        palette: control.palette
    }

    StyleReader {
        id: downProperties
        controlType: StyleReader.SpinBox
        enabled: control.enabled
        focused: control.activeFocus
        hovered: control.down.hovered
        pressed: control.down.pressed
        palette: control.palette
    }

    StyleKitLayout {
        id: spinBoxLayout
        container: control
        contentMargins {
            // Copy the other styles, and add indicator width to padding
            left: styleReader.leftPadding
            right: styleReader.rightPadding
            top: styleReader.topPadding
            bottom: styleReader.bottomPadding
        }
        layoutItems: [
            // We don't lay out the contentItem here because it occupies the remaining space
            // as calculated by control internal logic.
            StyleKitLayoutItem {
                id: upIndicatorItem
                item: control.up.indicator
                alignment: styleReader.indicator.up.alignment
                margins.left: styleReader.indicator.up.leftMargin
                margins.right: styleReader.indicator.up.rightMargin
                margins.top: styleReader.indicator.up.topMargin
                margins.bottom: styleReader.indicator.up.bottomMargin
                fillWidth: styleReader.indicator.up.implicitWidth === Style.Stretch
                fillHeight: styleReader.indicator.up.implicitHeight === Style.Stretch
            },
            StyleKitLayoutItem {
                id: downIndicatorItem
                item: control.down.indicator
                alignment: styleReader.indicator.down.alignment
                margins.left: styleReader.indicator.down.leftMargin
                margins.right: styleReader.indicator.down.rightMargin
                margins.top: styleReader.indicator.down.topMargin
                margins.bottom: styleReader.indicator.down.bottomMargin
                fillWidth: styleReader.indicator.down.implicitWidth === Style.Stretch
                fillHeight: styleReader.indicator.down.implicitHeight === Style.Stretch
            }
        ]
        spacing: styleReader.spacing
        mirrored: control.mirrored
    }

    validator: IntValidator {
        locale: control.locale.name
        bottom: Math.min(control.from, control.to)
        top: Math.max(control.from, control.to)
    }

    contentItem: TextInput {
        z: 2
        text: control.displayText
        font: control.font
        selectionColor: control.palette.highlight
        selectedTextColor: control.palette.highlightedText
        color: styleReader.text.color
        horizontalAlignment: styleReader.text.alignment & Qt.AlignHorizontal_Mask
        verticalAlignment: styleReader.text.alignment & Qt.AlignVertical_Mask
        padding: styleReader.text.padding
        topPadding: styleReader.text.topPadding
        bottomPadding: styleReader.text.bottomPadding
        leftPadding: styleReader.text.leftPadding
        rightPadding: styleReader.text.rightPadding

        readOnly: !control.editable
        validator: control.validator
        inputMethodHints: control.inputMethodHints
        clip: width < implicitWidth
    }

    up.indicator: IndicatorDelegate {
        quickControl: control
        indicatorStyle: upProperties.indicator.up
        x: upIndicatorItem.x
        y: upIndicatorItem.y
        width: upIndicatorItem.width
        height: upIndicatorItem.height
    }

    down.indicator: IndicatorDelegate {
        quickControl: control
        indicatorStyle: downProperties.indicator.down
        x: downIndicatorItem.x
        y: downIndicatorItem.y
        width: downIndicatorItem.width
        height: downIndicatorItem.height
    }

    background: BackgroundDelegate {
        quickControl: control
        backgroundStyle: styleReader.background
    }
}
