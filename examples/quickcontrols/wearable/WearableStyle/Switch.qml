// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.Switch {
    id: control

    implicitWidth: 48
    implicitHeight: 26

    indicator: Rectangle {
        x: control.visualPosition * (control.width - width)
        y: (control.height - height) / 2
        width: 26
        height: 26

        radius: 13
        color: control.down ? UIStyle.themeColorQtGray6 : UIStyle.themeColorQtGray10
        border.color: !control.checked ? "#999999"
                                       : (control.down ? UIStyle.colorQtAuxGreen2
                                                       : UIStyle.colorQtAuxGreen1)

        Behavior on x {
            enabled: !control.pressed
            SmoothedAnimation { velocity: 200 }
        }
    }

    background: Rectangle {
        radius: 13
        color: control.checked ? UIStyle.colorQtAuxGreen2 : UIStyle.colorRed
        border.color: control.checked ? UIStyle.colorQtAuxGreen2
                                      : UIStyle.themeColorQtGray6
    }
}

