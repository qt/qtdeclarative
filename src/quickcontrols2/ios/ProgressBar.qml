// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Controls.iOS

T.ProgressBar {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    opacity: control.enabled ? 1 : 0.5

    contentItem: Item {
        parent: control.background
        implicitWidth: progress.width
        implicitHeight: progress.implicitHeight
        scale: control.mirrored ? -1 : 1

        readonly property NinePatchImage progress: NinePatchImage {
            parent: control.contentItem
            visible: control.indeterminate || control.value
            y: (parent.height - height) / 2
            width: control.indeterminate ? control.width * 0.4 : control.position * parent.width

            source: control.IOS.url + "slider-progress"
            NinePatchImageSelector on source {
                states: [
                    {"light": control.IOS.theme === IOS.Light},
                    {"dark": control.IOS.theme === IOS.Dark}
                ]
            }

            NumberAnimation on x {
                running: control.indeterminate && control.visible
                from: -control.contentItem.progress.width
                to: control.width
                duration: 900
                easing.type: Easing.Linear
                loops: Animation.Infinite
                // TODO: workaround for QTBUG-38932; remove once that is fixed
                onFromChanged: {
                    if (control.indeterminate)
                        restart()
                }
                onToChanged: {
                    if (control.indeterminate)
                        restart()
                }
            }
        }
    }

    background: Item {
        implicitWidth: 150
        implicitHeight: children[0].implicitHeight
        clip: control.indeterminate
        NinePatchImage {
            source: control.IOS.url + "slider-background"
            y: (parent.height - height) / 2
            width: control.background.width
            NinePatchImageSelector on source {
                states: [
                    {"light": control.IOS.theme === IOS.Light},
                    {"dark": control.IOS.theme === IOS.Dark}
                ]
            }
        }
    }
}
