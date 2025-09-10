// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Controls.macOS.impl

T.MenuItem {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    leftPadding: 10
    rightPadding: 10
    topPadding: 3
    bottomPadding: 3
    spacing: 0

    icon.width: 16
    icon.height: 16

    implicitTextPadding: control.checkable && control.indicator ? control.indicator.width + control.spacing : 0

    contentItem: IconLabel {
        readonly property real arrowPadding: control.subMenu && control.arrow ? control.arrow.width + control.spacing : 0
        leftPadding: !control.mirrored ? control.textPadding : arrowPadding
        rightPadding: control.mirrored ? control.textPadding : arrowPadding

        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        alignment: Qt.AlignLeft

        icon: control.icon
        text: control.text
        font: control.font
        color: control.down || control.highlighted ? "white" : control.palette.text
    }

    arrow: ColorImage {
        x: control.mirrored ? control.padding : control.width - width - control.padding
        y: control.topPadding + (control.availableHeight - height) / 2
        width: 20

        visible: control.subMenu
        rotation: control.mirrored ? -180 : 0
        scale: 0.6
        color: control.palette.text
        source: "qrc:/qt-project.org/imports/QtQuick/Controls/macOS/images/menuarrow.png"
        fillMode: Image.Pad
    }

    indicator: CheckIndicator {
        x: control.mirrored ? control.width - width - control.rightPadding : control.leftPadding
        y: control.topPadding + (control.availableHeight - height) / 2

        control: control
        visible: control.checkable
    }

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 25
        radius: 4

        color: control.palette.accent
        opacity: 0.7
        visible: control.down || control.highlighted
    }
}
