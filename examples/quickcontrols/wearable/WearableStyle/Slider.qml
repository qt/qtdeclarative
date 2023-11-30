// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.Slider {
    id: control

    implicitWidth: 200
    implicitHeight: 32

    handle: Rectangle {
        x: control.visualPosition * (control.width - width)
        y: (control.height - height) / 2
        width: 32
        height: width
        radius: width/2

        color: control.pressed ? UIStyle.buttonGrayPressed : UIStyle.buttonGray
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

    }

    background: Rectangle {
        y: (control.height - height) / 2
        height: 6
        radius: height / 2
        color: UIStyle.buttonBackground

        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            color: UIStyle.buttonProgress
            radius: height / 2
        }

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

