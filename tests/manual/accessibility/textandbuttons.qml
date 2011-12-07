/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0

Rectangle {
    id : rect
    width: 300
    height: 200

    Rectangle {
        width : 200
        height : 20

        id: button
        anchors.top : rect.top
        anchors.topMargin: 30
        property string text : "Click to activate"
        property int counter : 0

        Accessible.role : Accessible.Button

        function accessibleAction(action) {
            if (action == Qt.Press)
                buttonAction()
        }

        function buttonAction() {
            ++counter
            text = "clicked " + counter

            text2.x += 20
        }

        Text {
            id : text1
            anchors.fill: parent
            text : parent.text
        }

        MouseArea {
            id : mouseArea
            anchors.fill: parent
            onClicked: parent.buttonAction()
        }
    }

    Text {
        id : text2
        anchors.top: button.bottom
        anchors.topMargin: 50
        text : "Hello World " + x

        Behavior on x { PropertyAnimation { duration: 500 } }
    }
}
