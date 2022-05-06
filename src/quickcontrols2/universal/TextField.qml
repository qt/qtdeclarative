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

T.TextField {
    id: control

    implicitWidth: implicitBackgroundWidth + leftInset + rightInset
                   || Math.max(contentWidth, placeholder.implicitWidth) + leftPadding + rightPadding
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding,
                             placeholder.implicitHeight + topPadding + bottomPadding)

    // TextControlThemePadding + 2 (border)
    padding: 12
    topPadding: padding - 7
    rightPadding: padding - 4
    bottomPadding: padding - 5

    Universal.theme: activeFocus ? Universal.Light : undefined

    color: !enabled ? Universal.chromeDisabledLowColor : Universal.foreground
    selectionColor: Universal.accent
    selectedTextColor: Universal.chromeWhiteColor
    placeholderTextColor: !enabled ? Universal.chromeDisabledLowColor :
                                     activeFocus ? Universal.chromeBlackMediumLowColor :
                                                   Universal.baseMediumColor
    verticalAlignment: TextInput.AlignVCenter

    PlaceholderText {
        id: placeholder
        x: control.leftPadding
        y: control.topPadding
        width: control.width - (control.leftPadding + control.rightPadding)
        height: control.height - (control.topPadding + control.bottomPadding)

        text: control.placeholderText
        font: control.font
        color: control.placeholderTextColor
        visible: !control.length && !control.preeditText && (!control.activeFocus || control.horizontalAlignment !== Qt.AlignHCenter)
        verticalAlignment: control.verticalAlignment
        elide: Text.ElideRight
        renderType: control.renderType
    }

    background: Rectangle {
        implicitWidth: 60 // TextControlThemeMinWidth - 4 (border)
        implicitHeight: 28 // TextControlThemeMinHeight - 4 (border)

        border.width: 2 // TextControlBorderThemeThickness
        border.color: !control.enabled ? control.Universal.baseLowColor :
                       control.activeFocus ? control.Universal.accent :
                       control.hovered ? control.Universal.baseMediumColor : control.Universal.chromeDisabledLowColor
        color: control.enabled ? control.Universal.background : control.Universal.baseLowColor
    }
}
