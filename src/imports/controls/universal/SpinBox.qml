/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Controls module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.6
import Qt.labs.templates 1.0 as T
import Qt.labs.controls.universal 1.0

T.SpinBox {
    id: control

    implicitWidth: Math.max(background ? background.implicitWidth : 0,
                            contentItem.implicitWidth + 16 +
                            (up.indicator ? up.indicator.implicitWidth : 0) +
                            (down.indicator ? down.indicator.implicitWidth : 0))
    implicitHeight: Math.max(contentItem.implicitHeight + topPadding + bottomPadding,
                             background ? background.implicitHeight : 0,
                             up.indicator ? up.indicator.implicitHeight : 0,
                             down.indicator ? down.indicator.implicitHeight : 0)
    baselineOffset: contentItem.y + contentItem.baselineOffset

    // TextControlThemePadding + 2 (border)
    topPadding: 5
    bottomPadding: 7
    leftPadding: 12 + (control.mirrored ? (up.indicator ? up.indicator.width : 0) : (down.indicator ? down.indicator.width : 0))
    rightPadding: 8 + (control.mirrored ? (down.indicator ? down.indicator.width : 0) : (up.indicator ? up.indicator.width : 0))

    Universal.theme: activeFocus ? Universal.Light : undefined

    //! [validator]
    validator: IntValidator {
        locale: control.locale.name
        bottom: Math.min(control.from, control.to)
        top: Math.max(control.from, control.to)
    }
    //! [validator]

    //! [contentItem]
    contentItem: TextInput {
        text: control.textFromValue(control.value, control.locale)

        font: control.font
        color: !enabled ? control.Universal.chromeDisabledLowColor :
                activeFocus ? control.Universal.chromeBlackHighColor : control.Universal.baseHighColor
        selectionColor: control.Universal.accent
        selectedTextColor: control.Universal.chromeWhiteColor
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: TextInput.AlignVCenter
        renderType: Text.NativeRendering

        validator: control.validator
        inputMethodHints: Qt.ImhFormattedNumbersOnly
    }
    //! [contentItem]

    //! [up.indicator]
    up.indicator: Item {
        implicitWidth: 28
        height: parent.height + 4
        y: -2
        x: control.mirrored ? 0 : parent.width - width

        Rectangle {
            x: 2; y: 4
            width: parent.width - 4
            height: parent.height - 8
            color: !control.up.pressed ? "transparent" :
                   control.activeFocus ? control.Universal.accent
                                       : control.Universal.chromeDisabledLowColor
        }

        Image {
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            source: "image://universal/" + (control.mirrored ? "left" : "right") + "arrow/"
                    + (!control.enabled ? control.Universal.chromeDisabledLowColor :
                                          control.activeFocus ? control.Universal.chromeBlackHighColor : control.Universal.baseHighColor)
        }
    }
    //! [up.indicator]

    //! [down.indicator]
    down.indicator: Item {
        implicitWidth: 28
        height: parent.height + 4
        y: -2
        x: control.mirrored ? parent.width - width : 0

        Rectangle {
            x: 2; y: 4
            width: parent.width - 4
            height: parent.height - 8
            color: !control.down.pressed ? "transparent" :
                     control.activeFocus ? control.Universal.accent
                                         : control.Universal.chromeDisabledLowColor
        }

        Image {
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            source: "image://universal/" + (control.mirrored ? "right" : "left") + "arrow/"
                    + (!control.enabled ? control.Universal.chromeDisabledLowColor :
                                          control.activeFocus ? control.Universal.chromeBlackHighColor : control.Universal.baseHighColor)
        }
    }
    //! [down.indicator]

    //! [background]
    background: Rectangle {
        implicitWidth: 60 + 28 // TextControlThemeMinWidth - 4 (border)
        implicitHeight: 28 // TextControlThemeMinHeight - 4 (border)

        border.width: 2 // TextControlBorderThemeThickness
        border.color: !control.enabled ? control.Universal.baseLowColor :
                       control.activeFocus ? control.Universal.accent : control.Universal.chromeDisabledLowColor
        color: control.enabled ? control.Universal.altHighColor : control.Universal.baseLowColor
    }
    //! [background]
}
