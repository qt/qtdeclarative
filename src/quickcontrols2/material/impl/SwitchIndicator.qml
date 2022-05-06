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
import QtQuick.Templates as T
import QtQuick.Controls.Material
import QtQuick.Controls.Material.impl

Item {
    id: indicator
    implicitWidth: 38
    implicitHeight: 32

    property T.AbstractButton control
    property alias handle: handle

    Material.elevation: 1

    Rectangle {
        width: parent.width
        height: 14
        radius: height / 2
        y: parent.height / 2 - height / 2
        color: indicator.control.enabled ? (indicator.control.checked ? indicator.control.Material.switchCheckedTrackColor : indicator.control.Material.switchUncheckedTrackColor)
                               : indicator.control.Material.switchDisabledTrackColor
    }

    Rectangle {
        id: handle
        x: Math.max(0, Math.min(parent.width - width, indicator.control.visualPosition * parent.width - (width / 2)))
        y: (parent.height - height) / 2
        width: 20
        height: 20
        radius: width / 2
        color: indicator.control.enabled ? (indicator.control.checked ? indicator.control.Material.switchCheckedHandleColor : indicator.control.Material.switchUncheckedHandleColor)
                               : indicator.control.Material.switchDisabledHandleColor

        Behavior on x {
            enabled: !indicator.control.pressed
            SmoothedAnimation {
                duration: 300
            }
        }
        layer.enabled: indicator.Material.elevation > 0
        layer.effect: ElevationEffect {
            elevation: indicator.Material.elevation
        }
    }
}
