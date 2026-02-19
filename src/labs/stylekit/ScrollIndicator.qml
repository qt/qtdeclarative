// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

T.ScrollIndicator {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    leftPadding: styleReader.leftPadding
    topPadding: styleReader.topPadding
    rightPadding: styleReader.rightPadding
    bottomPadding: styleReader.bottomPadding

    leftInset: styleReader.background.leftMargin
    topInset: styleReader.background.topMargin
    rightInset: styleReader.background.rightMargin
    bottomInset: styleReader.background.bottomMargin

    spacing: styleReader.spacing

    StyleVariation.controlType: styleReader.controlType
    StyleReader {
        id: styleReader
        controlType: StyleReader.ScrollIndicator
        enabled: control.enabled
        focused: control.activeFocus
        hovered: control.hovered
        palette: control.palette
        vertical: !control.horizontal
    }

    contentItem: IndicatorDelegate {
        quickControl: control
        indicatorStyle: styleReader.indicator
        opacity: 0 // TODO: let style decide how, and when, to hide a ScrollIndicator

        Connections {
            target: control
            function onActiveChanged() {
                if (active) {
                    control.contentItem.fadeOutAnim.stop()
                    control.contentItem.opacity = 1
                } else {
                    control.contentItem.fadeOutAnim.start()
                }
            }
        }

        property Animation fadeOutAnim: SequentialAnimation { // TODO: factor out as a style property
            PauseAnimation { duration: 600 }
            NumberAnimation {
                target: control
                property: "contentItem.opacity"
                duration: 500
                to: 0
                easing.type: Easing.OutExpo
            }
        }
    }

    background: BackgroundDelegate {
        quickControl: control
        backgroundStyle: styleReader.background
    }
}
