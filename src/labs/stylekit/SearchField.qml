// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

T.SearchField {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             searchIndicator.implicitIndicatorHeight + topPadding + bottomPadding,
                             clearIndicator.implicitIndicatorHeight + topPadding + bottomPadding)

    leftPadding: searchFieldLayout.padding.left
    rightPadding: searchFieldLayout.padding.right
    topPadding: searchFieldLayout.padding.top
    bottomPadding: searchFieldLayout.padding.bottom

    leftInset: styleReader.background.leftMargin
    rightInset: styleReader.background.rightMargin
    topInset: styleReader.background.topMargin
    bottomInset: styleReader.background.bottomMargin

    spacing: styleReader.spacing

    font: styleReader.font

    StyleVariation.controlType: styleReader.controlType
    StyleReader {
        id: styleReader
        controlType: StyleReader.SearchField
        enabled: control.enabled
        focused: control.activeFocus
        hovered: control.hovered || control.searchIndicator.hovered || control.clearIndicator.hovered
        pressed: control.searchIndicator.pressed || control.clearIndicator.pressed
        palette: control.palette
    }

    StyleReader {
        id: searchProperties
        controlType: StyleReader.SearchField
        enabled: control.enabled
        focused: control.activeFocus
        hovered: control.searchIndicator.hovered
        pressed: control.searchIndicator.pressed
        palette: control.palette
    }

    StyleReader {
        id: clearProperties
        controlType: StyleReader.SearchField
        enabled: control.enabled
        focused: control.activeFocus
        hovered: control.clearIndicator.hovered
        pressed: control.clearIndicator.pressed
        palette: control.palette
    }

    StyleKitLayout {
        id: searchFieldLayout
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
                id: searchIndicatorItem
                item: control.searchIndicator.indicator
                alignment: styleReader.indicator.first.alignment
                margins.left: styleReader.indicator.first.leftMargin
                margins.right: styleReader.indicator.first.rightMargin
                margins.top: styleReader.indicator.first.topMargin
                margins.bottom: styleReader.indicator.first.bottomMargin
                fillWidth: styleReader.indicator.first.fillWidth
                fillHeight: styleReader.indicator.first.fillHeight
            },
            StyleKitLayoutItem {
                id: clearIndicatorItem
                item: control.clearIndicator.indicator
                alignment: styleReader.indicator.second.alignment
                margins.left: styleReader.indicator.second.leftMargin
                margins.right: styleReader.indicator.second.rightMargin
                margins.top: styleReader.indicator.second.topMargin
                margins.bottom: styleReader.indicator.second.bottomMargin
                fillWidth: styleReader.indicator.second.fillWidth
                fillHeight: styleReader.indicator.second.fillHeight
            }
        ]
        spacing: styleReader.spacing
        mirrored: control.mirrored
    }

    delegate: ItemDelegate {
        id: delegate

        required property var model
        required property int index

        width: ListView.view.width
        highlighted: control.highlightedIndex === index
        hoverEnabled: control.hoverEnabled
        palette.text: styleReader.text.color
        palette.highlightedText: control.palette.highlightedText

        contentItem: Text {
            text: delegate.model[control.textRole]
            font.weight: control.currentIndex === index ? Font.DemiBold : Font.Normal
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
    }

    searchIndicator.indicator: IndicatorDelegate {
        quickControl: control
        indicatorStyle: searchProperties.indicator.first
        x: searchIndicatorItem.x
        y: searchIndicatorItem.y
        width: searchIndicatorItem.width
        height: searchIndicatorItem.height
    }

    clearIndicator.indicator: IndicatorDelegate {
        quickControl: control
        indicatorStyle: clearProperties.indicator.second
        visible: control.text.length > 0
        x: clearIndicatorItem.x
        y: clearIndicatorItem.y
        width: clearIndicatorItem.width
        height: clearIndicatorItem.height
    }

    contentItem: T.TextField {
        padding: styleReader.text.padding
        leftPadding: styleReader.text.leftPadding
        rightPadding: styleReader.text.rightPadding
        topPadding: styleReader.text.topPadding
        bottomPadding: styleReader.text.bottomPadding

        text: control.text
        font: styleReader.font
        placeholderText: control.placeholderText

        PlaceholderText {
            id: placeholder
            x: parent.leftPadding
            y: parent.topPadding
            width: parent.width - parent.leftPadding - parent.rightPadding
            height: parent.height - parent.topPadding - parent.bottomPadding

            text: control.placeholderText
            font: parent.font
            color: styleReader.text.color
            visible: !parent.length && !parent.preeditText && (!parent.activeFocus || parent.horizontalAlignment !== Qt.AlignHCenter)
            verticalAlignment: parent.verticalAlignment
            elide: Text.ElideRight
            renderType: parent.renderType
        }

        selectByMouse: control.selectTextByMouse

        color: styleReader.text.color
        selectionColor: control.palette.highlight
        selectedTextColor: control.palette.highlightedText
        verticalAlignment: styleReader.text.alignment & Qt.AlignVertical_Mask
        horizontalAlignment: styleReader.text.alignment & Qt.AlignHorizontal_Mask
    }

    background: BackgroundDelegate {
        quickControl: control
        backgroundStyle: styleReader.background
    }

    // TODO: Use the Popup control as is for now,
    // later we might want to customize it seperately for searchfield using control "variations"
    popup: Popup {
        y: control.height
        width: control.width
        height: Math.min(contentItem.implicitHeight, control.Window.height - control.y - control.height - control.padding)

        palette.text: control.palette.text
        palette.windowText: control.palette.windowText
        palette.buttonText: control.palette.buttonTex
        palette.highlight: control.palette.highlight
        palette.highlightedText: control.palette.highlightedText

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
