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
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Templates as T
import QtQuick.NativeStyle as NativeStyle

T.Button {
    id: control

    readonly property bool __nativeBackground: background instanceof NativeStyle.StyleItem

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    leftPadding: __nativeBackground ? background.contentPadding.left : 5
    rightPadding: __nativeBackground ? background.contentPadding.right : 5
    topPadding: __nativeBackground ? background.contentPadding.top : 5
    bottomPadding: __nativeBackground ? background.contentPadding.bottom : 5

    background: NativeStyle.Button {
        control: control
        contentWidth: contentItem.implicitWidth
        contentHeight: contentItem.implicitHeight
    }

    icon.width: 24
    icon.height: 24
    icon.color: control.palette.buttonText

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display

        icon: control.icon
        text: control.text
        font: control.font
        color: control.palette.buttonText
    }
}
