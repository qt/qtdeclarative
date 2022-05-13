// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.iOS
import QtQuick.Controls.impl

T.ScrollIndicator {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    contentItem: NinePatchImage {
        source: control.IOS.url + "scrollindicator-handle"
        NinePatchImageSelector on source {
            states: [
                {"light": control.IOS.theme === IOS.Light},
                {"dark": control.IOS.theme === IOS.Dark},
                {"horizontal": control.horizontal},
                {"vertical": control.vertical}
            ]
        }
        width: control.availableWidth
        height: control.availableHeight
        visible: control.size < 1.0
        opacity: 0.0

        states: State {
            name: "active"
            when: control.active
            PropertyChanges { control.contentItem.opacity: 0.75 }
        }

        transitions: [
            Transition {
                from: "active"
                SequentialAnimation {
                    PauseAnimation { duration: 450 }
                    NumberAnimation { target: control.contentItem; duration: 200; property: "opacity"; to: 0.0 }
                }
            }
        ]
    }
}
