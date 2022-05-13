// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.NativeStyle as NativeStyle

NativeStyle.DefaultGroupBox {
    id: control
    font.pixelSize: background.styleFont(control).pixelSize
    label: Item {
        readonly property point labelPos : control.__nativeBackground
                                  ? background.labelPos
                                  : Qt.point(0,0)
        x: labelPos.x + background.x
        y: labelPos.y + background.y - (control.__nativeBackground ? background.groupBoxPadding.top : 0)
        width: children[0].implicitWidth
        height: children[0].implicitHeight
        Text {
            width: parent.width
            height: parent.height
            text: control.title
            font: control.font
            color: control.palette.windowText
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
    }
}
