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
import Qt.labs.controls.material 1.0
import QtGraphicalEffects 1.0

T.ComboBox {
    id: control

    implicitWidth: Math.max(background ? background.implicitWidth : 0,
                            contentItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(background ? background.implicitHeight : 0,
                             contentItem.implicitHeight + topPadding + bottomPadding)
    baselineOffset: contentItem.y + contentItem.baselineOffset

    spacing: 6
    padding: 12

    //! [delegate]
    delegate: MenuItem {
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
        color: control.enabled ? control.Material.primaryTextColor : control.Material.hintTextColor
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        rightPadding: 14 + control.spacing
    }
    //! [contentItem]

    //! [background]
    background: Rectangle {
        implicitWidth: 120
        implicitHeight: 32

        radius: 2
        color: control.Material.dialogColor

        Behavior on color {
            ColorAnimation {
                duration: 400
            }
        }

        layer.enabled: control.enabled
        layer.effect: DropShadow {
            verticalOffset: 1
            color: control.Material.dropShadowColor
            samples: control.pressed ? 15 : 9
            spread: 0.5
        }

        Image {
            x: parent.width - width - control.rightPadding
            y: (parent.height - height) / 2
            opacity: !control.enabled ? 0.5 : 1.0
            source: "qrc:/qt-project.org/imports/Qt/labs/controls/material/images/drop-indicator.png"
        }

        Rectangle {
            width: parent.width
            height: parent.height
            radius: parent.radius
            visible: control.activeFocus
            color: control.Material.checkBoxUncheckedRippleColor
        }
    }
    //! [background]

    //! [popup]
    popup: T.Popup {
        y: control.height
        implicitWidth: control.width
        implicitHeight: listview.contentHeight
        transformOrigin: Item.Top
        topMargin: 12
        bottomMargin: 12

        Material.theme: control.Material.theme
        Material.accent: control.Material.accent
        Material.primary: control.Material.primary

        enter: Transition {
            // grow_fade_in
            NumberAnimation { property: "scale"; from: 0.9; to: 1.0; easing.type: Easing.OutQuint; duration: 220 }
            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutCubic; duration: 150 }
        }

        exit: Transition {
            // shrink_fade_out
            NumberAnimation { property: "scale"; from: 1.0; to: 0.9; easing.type: Easing.OutQuint; duration: 220 }
            NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.OutCubic; duration: 150 }
        }

        contentItem: ListView {
            id: listview
            clip: true
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex

            T.ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            radius: 3
            color: control.Material.dialogColor

            layer.enabled: control.enabled
            layer.effect: DropShadow {
                verticalOffset: 1
                color: control.Material.dropShadowColor
                samples: 15
                spread: 0.5
            }
        }
    }
    //! [popup]
}
