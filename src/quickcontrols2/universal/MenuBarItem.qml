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
import QtQuick.Controls.Universal

T.MenuBarItem {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    padding: 12
    topPadding: padding - 1
    bottomPadding: padding + 1
    spacing: 12

    icon.width: 20
    icon.height: 20
    icon.color: !enabled ? Universal.baseLowColor : Universal.baseHighColor

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        alignment: Qt.AlignLeft

        icon: control.icon
        text: control.text
        font: control.font
        color: !control.enabled ? control.Universal.baseLowColor : control.Universal.baseHighColor
    }

    background: Rectangle {
        implicitWidth: 40
        implicitHeight: 40

        color: !control.enabled ? control.Universal.baseLowColor :
                control.down ? control.Universal.listMediumColor :
                control.highlighted ? control.Universal.listLowColor : "transparent"

        Rectangle {
            x: 1; y: 1
            width: parent.width - 2
            height: parent.height - 2

            visible: control.visualFocus
            color: control.Universal.accent
            opacity: control.Universal.theme === Universal.Light ? 0.4 : 0.6
        }
    }
}
