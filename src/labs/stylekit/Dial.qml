// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

T.Dial {
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
        controlType: StyleReader.Dial
        enabled: control.enabled
        focused: control.activeFocus
        hovered: control.hovered
        pressed: control.pressed
        palette: control.palette
    }

    handle: HandleDelegate {
        id: handleItem
        quickControl: control
        handleStyle: styleReader.handle

        x: control.background.indicator.x + (control.background.indicator.width - width) / 2
        y: control.background.indicator.y + (control.background.indicator.height - height) / 2
        transform: [
            Translate {
                y: -Math.min(control.background.indicator.width, control.background.indicator.height) / 2
                    + (handleItem.height / 2)
                    + styleReader.spacing
            },
            Rotation {
                angle: control.angle
                origin.x: handleItem.width / 2
                origin.y: handleItem.height / 2
            }
        ]
    }

    background: BackgroundAndIndicatorDelegate {
        quickControl: control
        indicatorStyle: styleReader.indicator
        backgroundStyle: styleReader.background
    }
}
