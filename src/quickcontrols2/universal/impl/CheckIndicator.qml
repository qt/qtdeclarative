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

Rectangle {
    id: indicator
    implicitWidth: 20
    implicitHeight: 20

    color: !control.enabled ? "transparent" :
            control.down && !partiallyChecked ? control.Universal.baseMediumColor :
            control.checkState === Qt.Checked ? control.Universal.accent : "transparent"
    border.color: !control.enabled ? control.Universal.baseLowColor :
                   control.down ? control.Universal.baseMediumColor :
                   control.checked ? control.Universal.accent : control.Universal.baseMediumHighColor
    border.width: 2 // CheckBoxBorderThemeThickness

    property Item control
    readonly property bool partiallyChecked: control.checkState === Qt.PartiallyChecked

    ColorImage {
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2

        visible: indicator.control.checkState === Qt.Checked
        color: !indicator.control.enabled ? indicator.control.Universal.baseLowColor : indicator.control.Universal.chromeWhiteColor
        source: "qrc:/qt-project.org/imports/QtQuick/Controls/Universal/images/checkmark.png"
    }

    Rectangle {
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        width: indicator.partiallyChecked ? parent.width / 2 : parent.width
        height: indicator.partiallyChecked ? parent.height / 2 : parent.height

        visible: !indicator.control.pressed && enabled && indicator.control.hovered || indicator.partiallyChecked
        color: !indicator.partiallyChecked ? "transparent" :
               !indicator.control.enabled ? indicator.control.Universal.baseLowColor :
                indicator.control.down ? indicator.control.Universal.baseMediumColor :
                indicator.control.hovered ? indicator.control.Universal.baseHighColor : indicator.control.Universal.baseMediumHighColor
        border.width: indicator.partiallyChecked ? 0 : 2 // CheckBoxBorderThemeThickness
        border.color: indicator.control.Universal.baseMediumLowColor
    }
}
