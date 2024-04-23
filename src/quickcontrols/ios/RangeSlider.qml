// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.iOS.impl
import QtQuick.Controls.impl

T.RangeSlider {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            first.implicitHandleWidth + leftPadding + rightPadding,
                            second.implicitHandleWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             first.implicitHandleHeight + topPadding + bottomPadding,
                             second.implicitHandleWidth + leftPadding + rightPadding)

    first.handle: Item {
        implicitWidth: children[0].implicitWidth - children[0].leftInset - children[0].rightInset
        implicitHeight: children[0].implicitWidth - children[0].topInset - children[0].bottomInset
        x: Math.round(control.leftPadding + (control.horizontal ? control.first.visualPosition * (control.availableWidth - width) : (control.availableWidth - width) / 2))
        y: Math.round(control.topPadding + (control.horizontal ? (control.availableHeight - height) / 2 : control.first.visualPosition * (control.availableHeight - height)))

        NinePatchImage {
            x: -leftInset
            y: -topInset
            source: IOS.url + "slider-handle"
            NinePatchImageSelector on source {
                states: [
                    {"light": Qt.styleHints.colorScheme === Qt.Light},
                    {"dark": Qt.styleHints.colorScheme === Qt.Dark},
                ]
            }
        }
    }

    second.handle: Item {
        implicitWidth: children[0].implicitWidth - children[0].leftInset - children[0].rightInset
        implicitHeight: children[0].implicitWidth - children[0].topInset - children[0].bottomInset
        x: Math.round(control.leftPadding + (control.horizontal ? control.second.visualPosition * (control.availableWidth - width) : (control.availableWidth - width) / 2))
        y: Math.round(control.topPadding + (control.horizontal ? (control.availableHeight - height) / 2 : control.second.visualPosition * (control.availableHeight - height)))

        NinePatchImage {
            x: -leftInset
            y: -topInset
            source: IOS.url + "slider-handle"
            NinePatchImageSelector on source {
                states: [
                    {"light": Qt.styleHints.colorScheme === Qt.Light},
                    {"dark": Qt.styleHints.colorScheme === Qt.Dark},
                ]
            }
        }
    }

    background: Item {
        implicitWidth: control.horizontal ? 114 : children[0].implicitHeight
        implicitHeight: control.horizontal ? children[0].implicitHeight : 114
        opacity: control.enabled ? 1 : 0.5

        NinePatchImage {
            source: IOS.url + "slider-background"
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            rotation: control.horizontal ? 0 : -90
            width: control.horizontal ? control.background.width : control.background.height
            NinePatchImageSelector on source {
                states: [
                    {"light": Qt.styleHints.colorScheme === Qt.Light},
                    {"dark": Qt.styleHints.colorScheme === Qt.Dark},
                ]
            }

            NinePatchImage {
                readonly property real handleWidth: control.first.handle ? control.first.handle.width : 0

                x: handleWidth / 2 + control.first.position * (parent.width - handleWidth)
                y: (parent.height - height) / 2
                width: control.second.position * (parent.width - handleWidth) - control.first.position * (parent.width - handleWidth)
                height: parent.height

                source: IOS.url + "slider-progress"
                NinePatchImageSelector on source {
                    states: [
                        {"light": Qt.styleHints.colorScheme === Qt.Light},
                        {"dark": Qt.styleHints.colorScheme === Qt.Dark},
                    ]
                }
            }
        }
    }
}
