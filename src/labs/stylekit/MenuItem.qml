// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

T.MenuItem {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    leftPadding: menuItemLayout.padding.left
    topPadding: menuItemLayout.padding.top
    rightPadding: menuItemLayout.padding.right
    bottomPadding: menuItemLayout.padding.bottom

    leftInset: styleReader.background.leftMargin
    topInset: styleReader.background.topMargin
    rightInset: styleReader.background.rightMargin
    bottomInset: styleReader.background.bottomMargin

    spacing: styleReader.spacing

    icon.width: 24
    icon.height: 24
    icon.color: styleReader.text.color

    font: styleReader.font

    StyleVariation.controlType: styleReader.controlType
    StyleReader {
        id: styleReader
        controlType: StyleReader.MenuItem
        enabled: control.enabled
        focused: control.activeFocus
        checked: control.checked
        hovered: control.hovered
        pressed: control.down
        highlighted: control.highlighted
        palette: control.palette
    }

    StyleKitLayout {
        id: menuItemLayout
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
                id: checkIndicatorItem
                item: control.indicator
                alignment: styleReader.indicator.first.alignment
                margins.left: styleReader.indicator.first.leftMargin
                margins.right: styleReader.indicator.first.rightMargin
                margins.top: styleReader.indicator.first.topMargin
                margins.bottom: styleReader.indicator.first.bottomMargin
                fillWidth: styleReader.indicator.first.implicitWidth === Style.Stretch
                fillHeight: styleReader.indicator.first.implicitHeight === Style.Stretch
            },
            StyleKitLayoutItem {
                id: arrowIndicatorItem
                item: control.arrow
                alignment: styleReader.indicator.second.alignment
                margins.left: styleReader.indicator.second.leftMargin
                margins.right: styleReader.indicator.second.rightMargin
                margins.top: styleReader.indicator.second.topMargin
                margins.bottom: styleReader.indicator.second.bottomMargin
                fillWidth: styleReader.indicator.second.implicitWidth === Style.Stretch
                fillHeight: styleReader.indicator.second.implicitHeight === Style.Stretch
            }
        ]
        spacing: styleReader.spacing
        mirrored: control.mirrored
    }

    contentItem: MenuItemIconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        icon: control.icon
        text: control.text
        font: control.font
        color: styleReader.text.color
        menuItem: control

        alignment: styleReader.text.alignment
        topPadding: styleReader.text.topPadding
        bottomPadding: styleReader.text.bottomPadding
        leftPadding: styleReader.text.leftPadding
        rightPadding: styleReader.text.rightPadding
    }

    indicator: IndicatorDelegate {
        quickControl: control
        indicatorStyle: styleReader.indicator.first
        visible: control.checked && control.checkable && indicatorStyle.visible
        x: checkIndicatorItem.x
        y: checkIndicatorItem.y
        width: checkIndicatorItem.width
        height: checkIndicatorItem.height
    }

    arrow: IndicatorDelegate {
        quickControl: control
        indicatorStyle: styleReader.indicator.second
        visible: control.subMenu && indicatorStyle.visible
        x: arrowIndicatorItem.x
        y: arrowIndicatorItem.y
        width: arrowIndicatorItem.width
        height: arrowIndicatorItem.height
    }

    background: BackgroundDelegate {
        quickControl: control
        backgroundStyle: styleReader.background
    }
}
