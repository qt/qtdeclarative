// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Item {
    id: gridelementtest
    anchors.fill: parent
    property string testtext: ""

    Grid {
        id: gridelement
        height: 120; width: 120; spacing: 5; columns: 2
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 15

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
