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
import QtQuick.Controls.impl
import QtQuick.Controls.Fusion
import QtQuick.Controls.Fusion.impl

Rectangle {
    id: indicator

    property Item control
    readonly property color pressedColor: Fusion.mergedColors(control.palette.base, control.palette.windowText, 85)
    readonly property color checkMarkColor: Qt.darker(control.palette.text, 1.2)

    implicitWidth: 14
    implicitHeight: 14

    radius: width / 2
    color: control.down ? indicator.pressedColor : control.palette.base
    border.color: control.visualFocus ? Fusion.highlightedOutline(control.palette)
                                      : Qt.darker(control.palette.window, 1.5)

    Rectangle {
        y: 1
        width: parent.width
        height: parent.height - 1
        radius: width / 2
        color: "transparent"
        border.color: Fusion.topShadow
        visible: indicator.control.enabled && !indicator.control.down
    }

    Rectangle {
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        width: parent.width / 2.32
        height: parent.height / 2.32
        radius: width / 2
        color: Color.transparent(indicator.checkMarkColor, 180 / 255)
        border.color: Color.transparent(indicator.checkMarkColor, 200 / 255)
        visible: indicator.control.checked
    }
}
