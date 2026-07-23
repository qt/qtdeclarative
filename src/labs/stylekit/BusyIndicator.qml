// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

T.BusyIndicator {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    leftPadding: styleReader.leftPadding
    rightPadding: styleReader.rightPadding
    topPadding: styleReader.topPadding
    bottomPadding: styleReader.bottomPadding

    leftInset: styleReader.background.leftMargin
    rightInset: styleReader.background.rightMargin
    topInset: styleReader.background.topMargin
    bottomInset: styleReader.background.bottomMargin

    spacing: styleReader.spacing

    StyleVariation.controlType: styleReader.controlType
    StyleReader {
        id: styleReader
        controlType: StyleReader.BusyIndicator
        enabled: control.enabled
        focused: control.activeFocus
        hovered: control.hovered
        palette: control.palette
    }

    StyleKitLayout {
        id: busyIndicatorLayout
        container: control.contentItem
        layoutItems: [
            StyleKitLayoutItem {
                id: indicatorItem
                item: indicator
                margins {
                    left: styleReader.indicator.leftMargin
                    right: styleReader.indicator.rightMargin
                    top: styleReader.indicator.topMargin
                    bottom: styleReader.indicator.bottomMargin
                }
                alignment: styleReader.indicator.alignment
                fillWidth: styleReader.indicator.fillWidth
                fillHeight: styleReader.indicator.fillHeight
            }
        ]
    }

    contentItem: Item {
        implicitWidth: Math.max(indicator.implicitWidth, busyIndicatorLayout.implicitWidth)
        implicitHeight: Math.max(indicator.implicitHeight, busyIndicatorLayout.implicitHeight)

        IndicatorDelegate {
            id: indicator
            quickControl: control
            indicatorStyle: styleReader.indicator
            x: indicatorItem.x
            y: indicatorItem.y
            width: indicatorItem.width
            height: indicatorItem.height
        }
    }

    background: BackgroundDelegate {
        quickControl: control
        backgroundStyle: styleReader.background
    }
}
