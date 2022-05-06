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
    id: panel

    property Item control
    property bool highlighted: control.highlighted

    visible: !control.flat || control.down || control.checked

    color: Fusion.buttonColor(control.palette, panel.highlighted, control.down || control.checked, control.hovered)
    gradient: control.down || control.checked ? null : buttonGradient

    Gradient {
        id: buttonGradient
        GradientStop {
            position: 0
            color: Fusion.gradientStart(Fusion.buttonColor(panel.control.palette, panel.highlighted, panel.control.down, panel.control.hovered))
        }
        GradientStop {
            position: 1
            color: Fusion.gradientStop(Fusion.buttonColor(panel.control.palette, panel.highlighted, panel.control.down, panel.control.hovered))
        }
    }

    radius: 2
    border.color: Fusion.buttonOutline(control.palette, panel.highlighted || control.visualFocus, control.enabled)

    Rectangle {
        x: 1; y: 1
        width: parent.width - 2
        height: parent.height - 2
        border.color: Fusion.innerContrastLine
        color: "transparent"
        radius: 2
    }
}
