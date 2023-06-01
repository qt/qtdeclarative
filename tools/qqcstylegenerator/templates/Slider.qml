// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.FigmaStyle
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.Slider {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitHandleWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitHandleHeight + topPadding + bottomPadding)

    topPadding: (control.horizontal ? config.topPadding : config.rightPadding) || 0
    leftPadding: (control.horizontal ? config.leftPadding : config.bottomPadding) || 0
    rightPadding: (control.horizontal ? config.rightPadding : config.topPadding) || 0
    bottomPadding: (control.horizontal ? config.bottomPadding : config.leftPadding) || 0

    topInset: (control.horizontal ? -config.topInset : config.rightInset) || 0
    leftInset: (control.horizontal ? -config.leftInset : config.bottomInset) || 0
    rightInset: (control.horizontal ? -config.rightInset : config.topInset) || 0
    bottomInset: (control.horizontal ? -config.bottomInset : config.leftInset) || 0

    readonly property string currentState: [
        !control.enabled && "disabled",
        control.visualFocus && "focused",
        control.enabled && !control.pressed && control.hovered && "hovered",
        control.pressed && "pressed"
    ].filter(Boolean).join("-") || "normal"
    readonly property var config: ConfigReader.controls.slider[currentState] || {}

    handle: BorderImage {
        x: Math.round(control.leftPadding + (control.horizontal ? control.visualPosition * (control.availableWidth - width) : (control.availableWidth - width) / 2))
        y: Math.round(control.topPadding + (control.horizontal ? (control.availableHeight - height) / 2 : control.visualPosition * (control.availableHeight - height)))

        source: control.config.handle?.export === "image"
                    ? Qt.resolvedUrl("images/" + control.config.handle.name)
                    : ""
        border {
            top: control.config.handle?.topOffset || 0
            bottom: control.config.handle?.bottomOffset || 0
            left: control.config.handle?.leftOffset || 0
            right: control.config.handle?.rightOffset || 0
        }
    }

    background: Item {
        implicitWidth: control.horizontal ? _background.implicitWidth : _background.implicitHeight
        implicitHeight: control.horizontal ? _background.implicitHeight : _background.implicitWidth

        property BorderImage _background: BorderImage {
            parent: control.background
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            width: control.horizontal ? background.width : background.height
            height: control.horizontal ? background.height : background.width
            rotation: control.horizontal ? 0 : -90
            scale: control.horizontal && control.mirrored ? -1 : 1
            source: control.config.background?.export === "image"
                ? Qt.resolvedUrl("images/" + control.config.background.name)
                : ""
            border {
                top: control.config.background?.topOffset || 0
                bottom: control.config.background?.bottomOffset || 0
                left: control.config.background?.leftOffset || 0
                right: control.config.background?.rightOffset || 0
            }

            property BorderImage groove: BorderImage {
                parent: control.background._background
                x: (control.horizontal ? control.leftPadding : control.bottomPadding) + ((control.horizontal ? control.availableWidth : control.availableHeight) - width) / 2
                y: (control.horizontal ? control.topPadding : control.rightPadding) + ((control.horizontal ? control.availableHeight : control.availableWidth) - height) / 2
                width: control.horizontal ? control.availableWidth : control.availableHeight
                height: implicitHeight
                source: control.config.groove?.export === "image"
                    ? Qt.resolvedUrl("images/" + control.config.groove.name)
                    : ""
                border {
                    top: control.config.groove?.topOffset || 0
                    bottom: control.config.groove?.bottomOffset || 0
                    left: control.config.groove?.leftOffset || 0
                    right: control.config.groove?.rightOffset || 0
                }

                property BorderImage track: BorderImage {
                    parent: control.background._background.groove
                    width: control.position * parent.width
                    height: parent.height
                    source: control.config.track?.export === "image"
                        ? Qt.resolvedUrl("images/" + control.config.track.name)
                        : ""
                    border {
                        top: control.config.track?.topOffset || 0
                        bottom: control.config.track?.bottomOffset || 0
                        left: control.config.track?.leftOffset || 0
                        right: control.config.track?.rightOffset || 0
                    }
                }
            }
        }
    }
}
