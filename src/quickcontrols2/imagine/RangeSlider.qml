/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.Imagine
import QtQuick.Controls.Imagine.impl

T.RangeSlider {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            first.implicitHandleWidth + leftPadding + rightPadding,
                            second.implicitHandleWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             first.implicitHandleHeight + topPadding + bottomPadding,
                             second.implicitHandleHeight + topPadding + bottomPadding)

    topPadding: background ? background.topPadding : 0
    leftPadding: background ? background.leftPadding : 0
    rightPadding: background ? background.rightPadding : 0
    bottomPadding: background ? background.bottomPadding : 0

    topInset: background ? -background.topInset || 0 : 0
    leftInset: background ? -background.leftInset || 0 : 0
    rightInset: background ? -background.rightInset || 0 : 0
    bottomInset: background ? -background.bottomInset || 0 : 0

    first.handle: Image {
        x: control.leftPadding + (control.horizontal ? control.first.visualPosition * (control.availableWidth - width) : (control.availableWidth - width) / 2)
        y: control.topPadding + (control.horizontal ? (control.availableHeight - height) / 2 : control.first.visualPosition * (control.availableHeight - height))

        source: control.Imagine.url + "rangeslider-handle"
        ImageSelector on source {
            states: [
                {"first": true},
                {"vertical": control.vertical},
                {"horizontal": control.horizontal},
                {"disabled": !control.enabled},
                {"pressed": control.first.pressed},
                {"focused": control.first.handle.activeFocus},
                {"mirrored": control.mirrored},
                {"hovered": control.first.hovered}
            ]
        }
    }

    second.handle: Image {
        x: control.leftPadding + (control.horizontal ? control.second.visualPosition * (control.availableWidth - width) : (control.availableWidth - width) / 2)
        y: control.topPadding + (control.horizontal ? (control.availableHeight - height) / 2 : control.second.visualPosition * (control.availableHeight - height))

        source: control.Imagine.url + "rangeslider-handle"
        ImageSelector on source {
            states: [
                {"second": true},
                {"vertical": control.vertical},
                {"horizontal": control.horizontal},
                {"disabled": !control.enabled},
                {"pressed": control.second.pressed},
                {"focused": control.second.handle.activeFocus},
                {"mirrored": control.mirrored},
                {"hovered": control.second.hovered}
            ]
        }
    }

    background: NinePatchImage {
        scale: control.horizontal && control.mirrored ? -1 : 1

        source: control.Imagine.url + "rangeslider-background"
        NinePatchImageSelector on source {
            states: [
                {"vertical": control.vertical},
                {"horizontal": control.horizontal},
                {"disabled": !control.enabled},
                {"focused": control.visualFocus},
                {"mirrored": control.mirrored},
                {"hovered": control.hovered}
            ]
        }

        NinePatchImage {
            x: control.horizontal ? control.first.handle.width / 2 + control.first.position * (parent.width - control.first.handle.width) : (parent.width - width) / 2
            y: control.horizontal ? (parent.height - height) / 2 : control.first.handle.height / 2 + control.second.visualPosition * (parent.height - control.first.handle.height)
            width: control.horizontal ? control.second.position * (parent.width - control.first.handle.width) - control.first.position * (parent.width - control.first.handle.width) : parent.width
            height: control.vertical ? control.second.position * (parent.height - control.first.handle.height) - control.first.position * (parent.height - control.first.handle.height): parent.height

            source: control.Imagine.url + "rangeslider-progress"
            NinePatchImageSelector on source {
                states: [
                    {"vertical": control.vertical},
                    {"horizontal": control.horizontal},
                    {"disabled": !control.enabled},
                    {"focused": control.visualFocus},
                    {"mirrored": control.mirrored},
                    {"hovered": control.hovered}
                ]
            }
        }
    }
}
