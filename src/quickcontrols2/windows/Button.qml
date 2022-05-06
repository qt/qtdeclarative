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
import QtQuick.NativeStyle as NativeStyle

NativeStyle.DefaultButton {
    id: control

    background: NativeStyle.Button {
        control: control
        contentWidth: contentItem.implicitWidth
        contentHeight: contentItem.implicitHeight
        useNinePatchImage: false
        overrideState: NativeStyle.StyleItem.NeverHovered
    }

    NativeStyle.Button {
        id: hoverButton
        control: control
        x: background.x
        y: background.y
        width: background.width
        height: background.height
        useNinePatchImage: false
        overrideState: NativeStyle.StyleItem.AlwaysHovered
        opacity: control.hovered ? 1 : 0
        visible: opacity !== 0
        Behavior on opacity { NumberAnimation { duration: hoverButton.transitionDuration } }
    }

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display

        icon: control.icon
        text: control.text
        font: control.font
        color: control.flat && !control.down ? (control.visualFocus ? control.palette.highlight : control.palette.windowText) : control.palette.buttonText
    }
}
