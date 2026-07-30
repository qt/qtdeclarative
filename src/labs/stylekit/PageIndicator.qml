// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

T.PageIndicator {
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
        controlType: StyleReader.PageIndicator
        enabled: control.enabled
        focused: control.activeFocus
        hovered: control.hovered
        palette: control.palette
    }

    delegate: Item {
        id: pageIndicatorCell

        required property int index
        property bool __pressed: pressed

        implicitWidth: pageIndicatorDelegate.implicitWidth
        height: parent.height

        IndicatorDelegate {
            id: pageIndicatorDelegate
            y: (parent.height - height) / 2

            quickControl: control
            indicatorStyle: delegateStyleReader.indicator

            HoverHandler {
                id: hoverHandler
                enabled: control.interactive
            }

            StyleReader {
                id: delegateStyleReader
                controlType: StyleReader.PageIndicator
                enabled: control.enabled
                checked: pageIndicatorCell.index === control.currentIndex
                pressed: pageIndicatorCell.__pressed
                hovered: hoverHandler.hovered
                palette: control.palette
            }
        }
    }

    contentItem: Row {
        spacing: control.spacing

        Repeater {
            model: control.count
            delegate: control.delegate
        }
    }

    background: BackgroundDelegate {
        quickControl: control
        backgroundStyle: styleReader.background
    }
}
