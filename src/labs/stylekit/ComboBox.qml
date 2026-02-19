// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

T.ComboBox {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    leftPadding: comboLayout.padding.left
    rightPadding: comboLayout.padding.right
    topPadding: comboLayout.padding.top
    bottomPadding: comboLayout.padding.bottom

    leftInset: styleReader.background.leftMargin
    topInset: styleReader.background.topMargin
    rightInset: styleReader.background.rightMargin
    bottomInset: styleReader.background.bottomMargin

    spacing: styleReader.spacing

    font: styleReader.font

    StyleVariation.controlType: styleReader.controlType
    StyleReader {
        id: styleReader
        controlType: StyleReader.ComboBox
        enabled: control.enabled
        focused: control.activeFocus
        hovered: control.hovered || control.pressed
        pressed: control.down
        highlighted: control.highlighted
        palette: control.palette
    }

    StyleKitLayout {
        id: comboLayout
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
                alignment: Qt.AlignRight | Qt.AlignVCenter
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

    // TODO: Use the ItemDelegate control as is for now,
    // later we might want to customize it seperately for combobox using control "variations"
    delegate: ItemDelegate {
        required property var model
        required property int index

        width: ListView.view.width
        text: model[control.textRole]
        palette.text: control.palette.text
        palette.highlightedText: control.palette.highlightedText
        highlighted: control.highlightedIndex === index
        hoverEnabled: control.hoverEnabled
    }

    indicator: IndicatorDelegate {
        quickControl: control
        indicatorStyle: styleReader.indicator
        x: indicatorItem.x
        y: indicatorItem.y
        width: indicatorItem.width
        height: indicatorItem.height
    }

    contentItem: TextInput {
        text: control.editable ? control.editText : control.displayText
        font: control.font
        enabled: control.editable
        autoScroll: control.editable
        readOnly: control.down
        inputMethodHints: control.inputMethodHints
        validator: control.validator
        selectByMouse: control.selectTextByMouse
        selectionColor: control.palette.highlight
        selectedTextColor: control.palette.highlightedText
        color: styleReader.text.color
        verticalAlignment: styleReader.text.alignment & Qt.AlignVertical_Mask
        horizontalAlignment: styleReader.text.alignment & Qt.AlignHorizontal_Mask
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

    // TODO: Use the Popup control as is for now,
    // later we might want to customize it seperately for combobox using control "variations"
    popup: Popup {
        y: control.height
        width: control.width
        height: Math.min(contentItem.implicitHeight, control.Window.height - topMargin - bottomMargin)

        palette.text: control.palette.text
        palette.highlight: control.palette.highlight
        palette.highlightedText: control.palette.highlightedText
        palette.windowText: control.palette.windowText
        palette.buttonText: control.palette.buttonText

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.delegateModel
            currentIndex: control.highlightedIndex
            highlightMoveDuration: 0

            T.ScrollIndicator.vertical: ScrollIndicator { }
        }
    }
}
