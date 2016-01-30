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

T.ComboBox {
    id: control

    implicitWidth: Math.max(background ? background.implicitWidth : 0,
                            contentItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(background ? background.implicitHeight : 0,
                             contentItem.implicitHeight + topPadding + bottomPadding)
    baselineOffset: contentItem.y + contentItem.baselineOffset

    spacing: 8
    padding: 6
    leftPadding: 8
    rightPadding: 8

    //! [delegate]
    delegate: ItemDelegate {
        width: control.width
        text: control.textRole ? (Array.isArray(control.model) ? modelData[control.textRole] : model[control.textRole]) : modelData
        checkable: true
        autoExclusive: true
        checked: control.currentIndex === index
        highlighted: control.highlightedIndex === index
        pressed: highlighted && control.pressed
    }
    //! [delegate]

    //! [contentItem]
    contentItem: Text {
        text: control.displayText
        font: control.font
        color: "#ffffff"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        rightPadding: 18 + control.spacing
    }
    //! [contentItem]

    //! [background]
    background: Item {
        implicitWidth: 120
        implicitHeight: 40

        Rectangle {
            width: parent.width
            height: parent.height
            opacity: control.enabled ? 1.0 : 0.2
            color: control.pressed || popup.visible ? "#585A5C" : "#353637"
        }

        Image {
            x: parent.width - width - control.rightPadding
            y: (parent.height - height) / 2
            source: "qrc:/qt-project.org/imports/Qt/labs/controls/images/drop-indicator.png"
        }
    }
    //! [background]

    //! [popup]
    popup: T.Popup {
        y: control.height - 1
        implicitWidth: control.width
        implicitHeight: Math.min(396, listview.contentHeight)
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
                border.color: "#353637"
                color: "transparent"
            }

            T.ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle { }
    }
    //! [popup]
}
