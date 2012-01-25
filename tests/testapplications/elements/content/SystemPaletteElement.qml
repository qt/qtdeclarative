/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
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

Item {
    id: systempaletteelementtest
    anchors.fill: parent
    property string testtext: ""

    SystemPalette { id: syspal; colorGroup: SystemPalette.Inactive }
    Rectangle {
        height: parent.height *.8; width: parent.width *.8; border.width: 6; radius: 4
        anchors.centerIn: parent
        color: syspal.base; border.color: syspal.window
        Rectangle {
            height: 20; width: parent.width; border.color: "black"; color: syspal.window; radius: 4
            Text { text: "File"; color: syspal.windowText; font.pointSize: 9
                anchors { left: parent.left; leftMargin: 5; verticalCenter: parent.verticalCenter }
            }
            Rectangle {
                id: shadow
                height: button.height; width: button.width; color: syspal.shadow; radius: 5; opacity: .5
                anchors { left: button.left; top: button.top; leftMargin: 2; topMargin: 2 }
            }
            Rectangle {
                id: button
                width: 100; height: 30; radius: 5; border.color: "black"; color: clicky.pressed ? syspal.highlight : syspal.button
                Behavior on color { ColorAnimation { duration: 500 } }
                anchors { left: parent.left; top: parent.top; leftMargin: 10; topMargin: 30 }
                Text { anchors.centerIn: parent; text: "Button"; color: syspal.buttonText }
                MouseArea { id: clicky; anchors.fill: parent
                    onPressed: { shadow.anchors.topMargin = 1; shadow.anchors.leftMargin = 1 }
                    onReleased: { shadow.anchors.topMargin = 2; shadow.anchors.leftMargin = 2 }
                }
            }
        }
    }

    SystemTestHelp { id: helpbubble; visible: statenum != 0
        anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter; bottomMargin: 100 }
    }
    BugPanel { id: bugpanel }

    states: [
        State { name: "start"; when: statenum == 1
            PropertyChanges { target: systempaletteelementtest
                testtext: "This is an mock application shaded with the help of the SystemPalette element.\n"+
                "The colors of the menu bar, menu text and button should mimic that of the OS it is running on.\n"+
                "Pressing the lablelled button should shade it to the system highlight color." }
        }
    ]

}
