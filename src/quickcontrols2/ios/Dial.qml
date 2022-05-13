// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Controls.iOS
import QtQuick.Shapes

T.Dial {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    leftInset: handle ? handle.width / 2 : 0
    rightInset: handle ? handle.width / 2 : 0
    topInset: handle ? handle.height / 2 : 0
    bottomInset: handle ? handle.height / 2 : 0

    background: Item {
        implicitWidth: 104
        implicitHeight: 104
        x: control.leftInset + (control.availableWidth - width) / 2
        y: control.topInset + (control.availableHeight - height) / 2

        Rectangle {
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            implicitWidth: parent.implicitWidth
            implicitHeight: parent.implicitHeight
            width: Math.max(50, Math.min(control.background.width, control.background.height))
            height: width
            color: "transparent"
            border.color: control.palette.mid
            border.width: 4
            radius: width * 0.5
            z: -1

            opacity: control.enabled? 1 : 0.5
        }

        Shape {
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            implicitWidth: parent.implicitWidth
            implicitHeight: parent.implicitHeight
            width: Math.max(50, Math.min(control.background.width, control.background.height))
            height: width
            layer.enabled: true
            layer.samples: 4

            ShapePath {
                fillColor: "transparent"
                strokeColor: control.palette.button
                strokeWidth: 4

                capStyle: ShapePath.RoundCap

                PathAngleArc {
                    centerX: control.background.children[0].width / 2
                    centerY: control.background.children[0].height / 2
                    radiusX: control.background.children[0].width / 2 - 2
                    radiusY: radiusX
                    startAngle: -230
                    sweepAngle: 140 + control.angle
                }
            }
        }
    }

    handle: Item {
        height: dialHandle.height - dialHandle.topInset - dialHandle.bottomInset
        width: dialHandle.width - dialHandle.rightInset - dialHandle.leftInset
        x: control.background.x + control.background.width / 2 - width / 2
        y: control.background.y + control.background.height / 2 - height / 2

        transform: [
            Translate {
                x: Math.cos((angle - 90) * Math.PI / 180) * Math.min(control.background.width, control.background.height) * 0.5;
                y: Math.sin((angle - 90) * Math.PI / 180) * Math.min(control.background.width, control.background.height) * 0.5;
            }
        ]

        readonly property NinePatchImage dialHandle: NinePatchImage {
            parent: control.handle
            x: -leftInset
            y: -topInset

            source: control.IOS.url + "slider-handle"
            NinePatchImageSelector on source {
                states: [
                    {"light": control.IOS.theme === IOS.Light},
                    {"dark": control.IOS.theme === IOS.Dark},
                    {"disabled": !control.enabled}
                ]
            }
        }
    }
}
