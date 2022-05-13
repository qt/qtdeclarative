// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Controls.Material
import QtQuick.Controls.Material.impl

T.RangeSlider {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            first.implicitHandleWidth + leftPadding + rightPadding,
                            second.implicitHandleWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             first.implicitHandleHeight + topPadding + bottomPadding,
                             second.implicitHandleHeight + topPadding + bottomPadding)

    padding: 6

    first.handle: SliderHandle {
        x: control.leftPadding + (control.horizontal ? control.first.visualPosition * (control.availableWidth - width) : (control.availableWidth - width) / 2)
        y: control.topPadding + (control.horizontal ? (control.availableHeight - height) / 2 : control.first.visualPosition * (control.availableHeight - height))
        value: control.first.value
        handleHasFocus: activeFocus
        handlePressed: control.first.pressed
        handleHovered: control.first.hovered
    }

    second.handle: SliderHandle {
        x: control.leftPadding + (control.horizontal ? control.second.visualPosition * (control.availableWidth - width) : (control.availableWidth - width) / 2)
        y: control.topPadding + (control.horizontal ? (control.availableHeight - height) / 2 : control.second.visualPosition * (control.availableHeight - height))
        value: control.second.value
        handleHasFocus: activeFocus
        handlePressed: control.second.pressed
        handleHovered: control.second.hovered
    }

    background: Rectangle {
        x: control.leftPadding + (control.horizontal ? 0 : (control.availableWidth - width) / 2)
        y: control.topPadding + (control.horizontal ? (control.availableHeight - height) / 2 : 0)
        implicitWidth: control.horizontal ? 200 : 48
        implicitHeight: control.horizontal ? 48 : 200
        width: control.horizontal ? control.availableWidth : 4
        height: control.horizontal ? 4 : control.availableHeight
        scale: control.horizontal && control.mirrored ? -1 : 1
        color: control.enabled ? Color.transparent(control.Material.accentColor, 0.33) : control.Material.sliderDisabledColor

        Rectangle {
            x: control.horizontal ? control.first.position * parent.width : 0
            y: control.horizontal ? 0 : control.second.visualPosition * parent.height
            width: control.horizontal ? control.second.position * parent.width - control.first.position * parent.width : 4
            height: control.horizontal ? 4 : control.second.position * parent.height - control.first.position * parent.height

            color: control.enabled ? control.Material.accentColor : control.Material.sliderDisabledColor
        }
    }
}
