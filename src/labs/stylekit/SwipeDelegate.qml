// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

T.SwipeDelegate {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    leftPadding: styleReader.leftPadding
    rightPadding: styleReader.rightPadding
    topPadding: styleReader.topPadding
    bottomPadding: styleReader.bottomPadding

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
        controlType: StyleReader.SwipeDelegate
        enabled: control.enabled
        focused: control.activeFocus
        checked: control.checked
        hovered: control.hovered
        pressed: control.pressed
        highlighted: control.highlighted
        palette: control.palette
    }

    swipe.transition: Transition {
        SmoothedAnimation {
            velocity: 3
            easing.type: Easing.InOutCubic
        }
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

    background: BackgroundDelegate {
        quickControl: control
        backgroundStyle: styleReader.background
    }
}
