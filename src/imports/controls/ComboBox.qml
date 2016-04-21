/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
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
import QtQuick.Controls 2.0
import QtQuick.Templates 2.0 as T

T.ComboBox {
    id: control

    implicitWidth: (background ? background.implicitWidth : contentItem.implicitWidth) + leftPadding + rightPadding
    implicitHeight: Math.max(background ? background.implicitHeight : 0, contentItem.implicitHeight + topPadding + bottomPadding)
    baselineOffset: contentItem.y + contentItem.baselineOffset

    spacing: 8
    padding: 6
    leftPadding: 12
    rightPadding: 12

    opacity: enabled ? 1 : 0.3

    //! [delegate]
    delegate: ItemDelegate {
        width: control.width
        text: control.textRole ? (Array.isArray(control.model) ? modelData[control.textRole] : model[control.textRole]) : modelData
        font.weight: control.currentIndex === index ? Font.DemiBold : Font.Normal
        highlighted: control.highlightedIndex == index
    }
    //! [delegate]

    //! [contentItem]
    contentItem: Text {
        text: control.displayText
        font: control.font
        color: control.activeKeyFocus ? "#0066ff" : "#353637"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        rightPadding: 18 + control.spacing
    }
    //! [contentItem]

    //! [background]
    background: Rectangle {
        implicitWidth: 120
        implicitHeight: 40

        color: control.activeKeyFocus ? (control.pressed ? "#cce0ff" : "#f0f6ff") :
            (control.pressed || popup.visible ? "#d6d6d6" : "#f6f6f6")
        border.color: control.activeKeyFocus ? "#0066ff" : "#353637"
        border.width: control.activeKeyFocus ? 2 : 1

        Image {
            x: parent.width - width - 4
            y: (parent.height - height) / 2
            source: "image://default/double-arrow/" + (control.activeKeyFocus ? "#0066ff" : "#353637")
        }
    }
    //! [background]

    //! [popup]
    popup: T.Popup {
        y: control.height - (control.activeKeyFocus ? 0 : 1)
        implicitWidth: control.width
        implicitHeight: listview.contentHeight
        topMargin: 6
        bottomMargin: 6

        contentItem: ListView {
            id: listview
            clip: true
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex

            Rectangle {
                z: 10
                parent: listview
                width: listview.width
                height: listview.height
                color: "transparent"
                border.color: "#353637"
            }

            T.ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle { }
    }
    //! [popup]
}
