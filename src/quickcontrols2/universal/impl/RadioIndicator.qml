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
import QtQuick.Controls.Universal

Rectangle {
    id: indicator
    implicitWidth: 20
    implicitHeight: 20
    radius: width / 2
    color: "transparent"
    border.width: 2 // RadioButtonBorderThemeThickness
    border.color:  control.checked ? "transparent" :
                  !control.enabled ? control.Universal.baseLowColor :
                   control.down ? control.Universal.baseMediumColor :
                   control.hovered ? control.Universal.baseHighColor : control.Universal.baseMediumHighColor

    property var control

    Rectangle {
        id: checkOuterEllipse
        width: parent.width
        height: parent.height

        radius: width / 2
        opacity: indicator.control.checked ? 1 : 0
        color: "transparent"
        border.width: 2 // RadioButtonBorderThemeThickness
        border.color: !indicator.control.enabled ? indicator.control.Universal.baseLowColor :
                       indicator.control.down ? indicator.control.Universal.baseMediumColor : indicator.control.Universal.accent
    }

    Rectangle {
        id: checkGlyph
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        width: parent.width / 2
        height: parent.height / 2

        radius: width / 2
        opacity: indicator.control.checked ? 1 : 0
        color: !indicator.control.enabled ? indicator.control.Universal.baseLowColor :
                indicator.control.down ? indicator.control.Universal.baseMediumColor :
                indicator.control.hovered ? indicator.control.Universal.baseHighColor : indicator.control.Universal.baseMediumHighColor
    }
}
