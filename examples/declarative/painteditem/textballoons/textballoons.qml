/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the demonstration applications of the Qt Toolkit.
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
import TextBalloonPlugin 1.0

Item {
    height: 480
    width: 640

    //! [0]
    ListModel {
        id: balloonModel
        ListElement {
            balloonWidth: 200
        }
        ListElement {
            balloonWidth: 350
        }
    }

    ListView {
        anchors.bottom: controls.top
        anchors.bottomMargin: 2
        anchors.top: parent.top
        id: balloonView
        delegate: TextBalloon {
            anchors.right: index % 2 == 0 ? undefined : parent.right
            height: 60
            rightAligned: index % 2 == 0 ? false : true
            width: balloonWidth
        }
        model: balloonModel
        spacing: 5
        width: parent.width
    }
    //! [0]

    //! [1]
    Rectangle {
        id: controls
 
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.margins: 1
        anchors.right: parent.right
        border.width: 2
        color: "white"
        height: parent.height * 0.15

        Text {
            anchors.centerIn: parent
            text: "Add another balloon"
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                balloonModel.append({"balloonWidth": Math.floor(Math.random() * 300 + 100)})
                balloonView.positionViewAtIndex(balloonView.count -1, ListView.End)
            }
            onEntered: {
                parent.color = "#8ac953"
            }
            onExited: {
                parent.color = "white"
            }
        }
    }
    //! [1]
}
