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
import QtQuick.Controls.impl
import QtQuick.Controls.Fusion
import QtQuick.Controls.Fusion.impl

T.TabButton {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: 2
    horizontalPadding: 4
    spacing: 6

    icon.width: 16
    icon.height: 16

    z: checked

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display

        icon: control.icon
        text: control.text
        font: control.font
        color: control.palette.buttonText
    }

    background: Rectangle {
        y: control.checked || control.TabBar.position !== T.TabBar.Header ? 0 : 2
        implicitHeight: 21
        height: control.height - (control.checked ? 0 : 2)

        border.color: Qt.lighter(Fusion.outline(control.palette), 1.1)

        gradient: Gradient {
            GradientStop {
                position: 0
                color: control.checked ? Qt.lighter(Fusion.tabFrameColor(control.palette), 1.04)
                                       : Qt.darker(Fusion.tabFrameColor(control.palette), 1.08)
            }
            GradientStop {
                position: control.checked ? 0 : 0.85
                color: control.checked ? Qt.lighter(Fusion.tabFrameColor(control.palette), 1.04)
                                       : Qt.darker(Fusion.tabFrameColor(control.palette), 1.08)
            }
            GradientStop {
                position: 1
                color: control.checked ? Fusion.tabFrameColor(control.palette)
                                       : Qt.darker(Fusion.tabFrameColor(control.palette), 1.16)
            }
        }
    }
}
