/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
    id: gridelementtest
    anchors.fill: parent
    property string testtext: ""

    Grid {
        id: gridelement
        height: 250; width: 120; spacing: 5; columns: 2
        anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: 100

        Rectangle { id: gr; color: "green"; height: 50; width: 50; border.color: "gray"; border.width: 3; opacity: .9; radius: 5; clip: true }
        Rectangle { id: re; color: "red"; height: 50; width: 50; border.color: "gray"; border.width: 3; opacity: .9; radius: 5; clip: true }
        Rectangle {
            id: bl
            color: "blue"; height: 50; width: 50; border.color: "gray"; border.width: 3; radius: 5; clip: true
            opacity: 0; visible: opacity != 0
        }
        Rectangle { id: bk; color: "black"; height: 50; width: 50; border.color: "gray"; border.width: 3; opacity: .9; radius: 5; clip: true }

        move: Transition { NumberAnimation { properties: "x,y"; duration: 1000; easing.type: Easing.OutBounce } }
        add: Transition { NumberAnimation { properties: "x,y"; duration: 1000; easing.type: Easing.OutBounce } }

    }

    SystemTestHelp { id: helpbubble; visible: statenum != 0
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter; topMargin: 50 }
    }
    BugPanel { id: bugpanel }

    states: [
        State { name: "start"; when: statenum == 1
            PropertyChanges { target: gridelementtest
                testtext: "This is a Grid element. At present it should be showing three rectangles - green, red and black, two "+
                "in the top row.\n"+
                "Next, let's add a rectangle to the Grid - it should drop in from the top and the black rectangle should move to give it space" }
        },
        State { name: "added"; when: statenum == 2
            PropertyChanges { target: bl; opacity: .9 }
            PropertyChanges { target: gridelementtest
                testtext: "The Grid should now be showing four rectangles - green, red, blue and black.\n"+
                "Next, lets alter the Grid to form a single column" }
        },
        State { name: "singlecolumn"; when: statenum == 3
            PropertyChanges { target: bl; opacity: .9 }
            PropertyChanges { target: gridelement; columns: 1 }
            PropertyChanges { target: gridelementtest
                testtext: "The Grid should now be showing four rectangles - green, red, blue and black.\n"+
                "Advance to restart the test." }
        }
    ]

}
