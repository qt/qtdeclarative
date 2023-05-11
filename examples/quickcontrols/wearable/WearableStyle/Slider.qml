// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.Slider {
    id: control

    implicitWidth: 200
    implicitHeight: 26

    handle: Rectangle {
        x: control.visualPosition * (control.width - width)
        y: (control.height - height) / 2
        width: 20
        height: 15

        radius: 5
        color: control.pressed ? "#f0f0f0" : "#f6f6f6"
        border.color: UIStyle.themeColorQtGray7
    }

    background: Rectangle {
        y: (control.height - height) / 2
        height: 4
        radius: 2
        color: UIStyle.themeColorQtGray3

        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            color: UIStyle.colorQtAuxGreen2
            radius: 2
        }
    }
}

