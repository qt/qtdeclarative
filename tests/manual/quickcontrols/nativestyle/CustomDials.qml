// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic.impl
import QtQuick.Layouts

ControlContainer {
    id: container
    title: "Dials"

    Row {
        spacing: container.rowSpacing

        Dial {
            id: dial1
            width: 50
            height: 50
            from: 0
            to: 10
            value: 5

            background: DialImpl {
                implicitWidth: 184
                implicitHeight: 184
                color: "darkgray"
                progress: dial1.position
                opacity: dial1.enabled ? 1 : 0.3
            }

            handle: ColorImage {
                x: dial1.background.x + dial1.background.width / 2 - width / 2
                y: dial1.background.y + dial1.background.height / 2 - height / 2
                width: 14
                height: 10
                color: "green"
                source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/dial-indicator.png"
                antialiasing: true
                opacity: dial1.enabled ? 1 : 0.3
                transform: [
                    Translate {
                        y: -Math.min(dial1.background.width, dial1.background.height) * 0.4 + dial1.handle.height / 2
                    },
                    Rotation {
                        angle: dial1.angle
                        origin.x: dial1.handle.width / 2
                        origin.y: dial1.handle.height / 2
                    }
                ]
            }
        }
    }
}
