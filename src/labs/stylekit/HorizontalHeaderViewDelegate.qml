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
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    highlighted: control.selected

    readonly property var view: control.headerView.syncView
    readonly property bool indicatorVisible: control.headerView.showSortIndicator
                                             && view && model && view.sortColumn === model.index
    readonly property bool ascending: view && view.sortOrder === Qt.AscendingOrder

    leftPadding: horizontalHeaderViewLayout.padding.left
    rightPadding: horizontalHeaderViewLayout.padding.right
    topPadding: horizontalHeaderViewLayout.padding.top
    bottomPadding: horizontalHeaderViewLayout.padding.bottom

    leftInset: styleReader.background.leftMargin
    rightInset: styleReader.background.rightMargin
    topInset: styleReader.background.topMargin
    bottomInset: styleReader.background.bottomMargin

    spacing: styleReader.spacing

    font: styleReader.font

    StyleVariation.controlType: styleReader.controlType
    StyleReader {
        id: styleReader
        controlType: StyleReader.HorizontalHeaderViewDelegate
        enabled: control.enabled
        focused: control.activeFocus
        checked: control.checked
        hovered: control.hovered
        pressed: control.pressed
        highlighted: control.highlighted
        palette: control.palette
    }

    StyleKitLayout {
        id: horizontalHeaderViewLayout
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
                id: ascendingIndicatorItem
                item: control.indicator
                alignment: styleReader.indicator.first.alignment
                margins.left: styleReader.indicator.first.leftMargin
                margins.right: styleReader.indicator.first.rightMargin
                margins.top: styleReader.indicator.first.topMargin
                margins.bottom: styleReader.indicator.first.bottomMargin
                fillWidth: styleReader.indicator.first.fillWidth
                fillHeight: styleReader.indicator.first.fillHeight
            },
            StyleKitLayoutItem {
                id: descendingIndicatorItem
                item: descendingIndicator
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

    contentItem: Label {
        text: control.model[control.headerView.textRole]
        elide: Text.ElideRight
        color: styleReader.text.color
        font: styleReader.font

        horizontalAlignment: styleReader.text.alignment & Qt.AlignHorizontal_Mask
        verticalAlignment: styleReader.text.alignment & Qt.AlignVertical_Mask
        padding: styleReader.text.padding
        leftPadding: styleReader.text.leftPadding
        rightPadding: styleReader.text.rightPadding
        bottomPadding: styleReader.text.bottomPadding
        topPadding: styleReader.text.topPadding
    }

    indicator: IndicatorDelegate {
        quickControl: control
        indicatorStyle: styleReader.indicator.first
        visible: control.indicatorVisible && control.ascending && indicatorStyle.visible
        x: ascendingIndicatorItem.x
        y: ascendingIndicatorItem.y
        width: ascendingIndicatorItem.width
        height: ascendingIndicatorItem.height
    }

    IndicatorDelegate {
        id: descendingIndicator
        quickControl: control
        indicatorStyle: styleReader.indicator.second
        visible: control.indicatorVisible && !control.ascending && indicatorStyle.visible
        x: descendingIndicatorItem.x
        y: descendingIndicatorItem.y
        width: descendingIndicatorItem.width
        height: descendingIndicatorItem.height
    }

    background: BackgroundDelegate {
        quickControl: control
        backgroundStyle: styleReader.background
    }
}
