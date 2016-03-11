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
import QtQuick.Window 2.2
import Qt.labs.templates 1.0 as T
import Qt.labs.controls.universal 1.0

T.ComboBox {
    id: control

    implicitWidth: Math.max(background ? background.implicitWidth : 0,
                            contentItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(background ? background.implicitHeight : 0,
                             contentItem.implicitHeight + topPadding + bottomPadding)
    baselineOffset: contentItem.y + contentItem.baselineOffset

    spacing: 10
    topPadding: 5
    leftPadding: 12
    rightPadding: 10
    bottomPadding: 7

    //! [delegate]
    delegate: ItemDelegate {
        width: control.width
        text: control.textRole ? (Array.isArray(control.model) ? modelData[control.textRole] : model[control.textRole]) : modelData
        highlighted: control.highlightedIndex === index
        pressed: highlighted && control.pressed
    }
    //! [delegate]

    //! [contentItem]
    contentItem: Text {
        text: control.displayText
        font: control.font
        color: !control.enabled ? control.Universal.baseLowColor : control.Universal.baseHighColor
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        rightPadding: 12 + control.spacing
    }
    //! [contentItem]

    //! [background]
    background: Rectangle {
        implicitWidth: 120
        implicitHeight: 32

        border.width: 2 // ComboBoxBorderThemeThickness
        border.color: !control.enabled ? control.Universal.baseLowColor :
                       control.pressed || popup.visible ? control.Universal.baseMediumLowColor : control.Universal.baseMediumLowColor
        color: !control.enabled ? control.Universal.baseLowColor :
                control.pressed || popup.visible ? control.Universal.listMediumColor : control.Universal.altMediumLowColor

        Rectangle {
            x: 2
            y: 2
            width: parent.width - 4
            height: parent.height - 4

            visible: control.activeFocus && (control.focusReason === Qt.TabFocusReason || control.focusReason === Qt.BacktabFocusReason)
            color: control.Universal.accent
            opacity: control.Universal.theme === Universal.Light ? 0.4 : 0.6
        }

        Image {
            id: checkmark
            x: parent.width - width - control.rightPadding
            y: (parent.height - height) / 2
            source: "image://universal/downarrow/" + (!control.enabled ? control.Universal.baseLowColor : control.Universal.baseMediumHighColor)
        }
    }
    //! [background]

    //! [popup]
    popup: T.Popup {
        implicitWidth: control.width
        implicitHeight: Math.min(396, listview.contentHeight)
        topMargin: 8
        bottomMargin: 8

        Universal.theme: control.Universal.theme
        Universal.accent: control.Universal.accent

        contentItem: ListView {
            id: listview
            clip: true
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex

            T.ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            color: control.Universal.chromeMediumLowColor
            border.color: control.Universal.chromeHighColor
            border.width: 1 // FlyoutBorderThemeThickness
        }
    }
    //! [popup]
}
