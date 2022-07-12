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
    id: handle

    property var palette
    property bool pressed
    property bool hovered
    property bool vertical
    property bool visualFocus

    implicitWidth: 13
    implicitHeight: 13

    gradient: Gradient {
        GradientStop {
            position: 0
            color: Fusion.gradientStart(Fusion.buttonColor(handle.palette, handle.visualFocus,
                handle.pressed, handle.enabled && handle.hovered))
        }
        GradientStop {
            position: 1
            color: Fusion.gradientStop(Fusion.buttonColor(handle.palette, handle.visualFocus,
                handle.pressed, handle.enabled && handle.hovered))
        }
    }
    rotation: handle.vertical ? -90 : 0
    border.width: 1
    border.color: "transparent"
    radius: 2

    Rectangle {
        width: parent.width
        height: parent.height
        border.color: handle.visualFocus ? Fusion.highlightedOutline(handle.palette) : Fusion.outline(handle.palette)
        color: "transparent"
        radius: 2

        Rectangle {
            x: 1; y: 1
            width: parent.width - 2
            height: parent.height - 2
            border.color: Fusion.innerContrastLine
            color: "transparent"
            radius: 2
        }
    }
}
