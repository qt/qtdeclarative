// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Controls.iOS.impl
import QtQuick.Shapes

T.Dial {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    // The handle extends outside background bounds,
    // so we need to include that in the insets calculation
    leftInset: handle ? handle.width / 2 : 0
    rightInset: handle ? handle.width / 2 : 0
    topInset: handle ? handle.height / 2 : 0
    bottomInset: handle ? handle.height / 2 : 0

    background: Item {
        implicitWidth: _groove.implicitWidth
        implicitHeight: _groove.implicitHeight

        readonly property int _strokeWidth: 4

        readonly property Rectangle _groove: Rectangle {
            parent: control.background
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            implicitWidth: 104
            implicitHeight: 104
            width: Math.min(parent.width, parent.height)
            height: width
            color: "transparent"
            border.color: control.palette.mid
            border.width: control.background._strokeWidth
            radius: width * 0.5
            z: -1
            opacity: control.enabled ? 1 : 0.5
        }

        readonly property Shape _track: Shape {
            parent: control.background
            x: control.background._groove.x
            y: control.background._groove.y
            width: control.background._groove.width
            height: control.background._groove.height
            layer.enabled: true
            layer.samples: 4

            ShapePath {
                fillColor: "transparent"
                strokeColor: control.palette.button
                strokeWidth: 4
                capStyle: ShapePath.RoundCap

                PathAngleArc {
                    centerX: control.background._track.width / 2
                    centerY: control.background._track.height / 2
                    radiusX: (control.background._track.width - control.background._strokeWidth) / 2
                    radiusY: radiusX
                    startAngle: control.startAngle - 90
                    sweepAngle: control.angle - control.startAngle
                }
            }
        }
    }

    handle: Item {
        height: dialHandle.height - dialHandle.topInset - dialHandle.bottomInset
        width: dialHandle.width - dialHandle.rightInset - dialHandle.leftInset
        x: control.background ? control.background.x + (control.background.width - width) / 2 : 0
        y: control.background ? control.background.y + (control.background.height - height) / 2 : 0

        transform: [
            Translate {
                readonly property real radius: !control.background ? 0
                    : (Math.min(control.background.width, control.background.height)
                        - control.background._strokeWidth) / 2
                x: Math.cos((angle - 90) * Math.PI / 180) * radius
                y: Math.sin((angle - 90) * Math.PI / 180) * radius
            }
        ]

        readonly property NinePatchImage dialHandle: NinePatchImage {
            parent: control.handle
            x: -leftInset
            y: -topInset

            source: IOS.url + "slider-handle"
            NinePatchImageSelector on source {
                states: [
                    {"light": Application.styleHints.colorScheme === Qt.Light},
                    {"dark": Application.styleHints.colorScheme === Qt.Dark},
                    {"disabled": !control.enabled}
                ]
            }
        }
    }
}
