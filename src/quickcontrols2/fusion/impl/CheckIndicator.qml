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

    color: control.down ? indicator.pressedColor : control.palette.base
    border.color: control.visualFocus ? Fusion.highlightedOutline(control.palette)
                                      : Qt.lighter(Fusion.outline(control.palette), 1.1)

    Rectangle {
        x: 1; y: 1
        width: parent.width - 2
        height: 1
        color: Fusion.topShadow
        visible: indicator.control.enabled && !indicator.control.down
    }

    ColorImage {
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        color: Color.transparent(indicator.checkMarkColor, 210 / 255)
        source: "qrc:/qt-project.org/imports/QtQuick/Controls/Fusion/images/checkmark.png"
        visible: indicator.control.checkState === Qt.Checked || (indicator.control.checked && indicator.control.checkState === undefined)
    }

    Rectangle {
        x: 3; y: 3
        width: parent.width - 6
        height: parent.width - 6

        visible: indicator.control.checkState === Qt.PartiallyChecked

        gradient: Gradient {
            GradientStop {
                position: 0
                color: Color.transparent(indicator.checkMarkColor, 80 / 255)
            }
            GradientStop {
                position: 1
                color: Color.transparent(indicator.checkMarkColor, 140 / 255)
            }
        }
        border.color: Color.transparent(indicator.checkMarkColor, 180 / 255)
    }
}
