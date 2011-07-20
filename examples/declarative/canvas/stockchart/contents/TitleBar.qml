/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0

Item {
    id: titleBar
    property string title: ""

    BorderImage { source: "images/titlebar.sci"; width: parent.width; height: parent.height + 14; y: -7 }

    Image {
        id: quitButton
        anchors.left: parent.left//; anchors.leftMargin: 0
        anchors.verticalCenter: parent.verticalCenter
        source: "images/quit.png"
        MouseArea {
            anchors.fill: parent
            onClicked: Qt.quit()
        }
    }

    Text {
        id: categoryText
        anchors {
            left: quitButton.right; right: parent.right; //leftMargin: 10; rightMargin: 10
            verticalCenter: parent.verticalCenter
        }
        elide: Text.ElideLeft
        text: title
        font.bold: true; font.pointSize: 20; color: "White"; style: Text.Raised; styleColor: "Black"
    }
}
