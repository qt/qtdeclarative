/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

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
        y: labelPos.y + background.y - groupBoxPadding.top
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
