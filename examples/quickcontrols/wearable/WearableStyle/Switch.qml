// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.Switch {
    id: control

    implicitWidth: 64
    implicitHeight: 34

    indicator: Rectangle {
        x: 2 + control.visualPosition * (control.width - width - 4)
        y: (control.height - height) / 2
        width: height
        height: parent.height - 4
        radius: height / 2

        color: control.down ? UIStyle.buttonGrayPressed : UIStyle.buttonGray
        border.color: UIStyle.buttonGrayOutLine

        Rectangle {
            width: parent.width
            height: parent.height
            radius: parent.radius
            gradient: Gradient {
                GradientStop { position: 0.0; color: UIStyle.gradientOverlay1 }
                GradientStop { position: 1.0; color: UIStyle.gradientOverlay2 }
            }
        }

        Behavior on x {
            enabled: !control.pressed
            SmoothedAnimation { velocity: 200 }
        }
    }

    background: Rectangle {
        radius: 17
        color: control.checked ? UIStyle.buttonProgress : UIStyle.buttonBackground

        Rectangle {
            width: parent.width
            height: parent.height
            radius: parent.radius
            gradient: Gradient {
                GradientStop { position: 0.0; color: UIStyle.gradientOverlay1 }
                GradientStop { position: 1.0; color: UIStyle.gradientOverlay2 }
            }
        }
    }
}

