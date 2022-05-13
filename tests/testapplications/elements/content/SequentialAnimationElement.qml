// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Item {
    id: sequentialanimationelementtest
    anchors.fill: parent
    property string testtext: ""
    property int firstduration: 1000
    property int secondduration: 3000
    property int firstY
    firstY: parent.height * .6
    property int secondY
    secondY: parent.height * .8

    Timer { id: startanimationtimer; interval: 1000; onTriggered: sequentialanimationelement.start() }

    SequentialAnimation {
        id: sequentialanimationelement
        running: false
        NumberAnimation { id: movement; target: animatedrect; properties: "y"; to: secondY; duration: firstduration }
        ColorAnimation { id: recolor; target: animatedrect; properties: "color"; to: "green"; duration: secondduration }
    }

    Rectangle {
        id: animatedrect
        width: 50; height: 50; color: "blue"; y: firstY
        anchors.horizontalCenter: parent.horizontalCenter
    }

    SystemTestHelp { id: helpbubble; visible: statenum != 0
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter; topMargin: 50 }
    }
    BugPanel { id: bugpanel }

    states: [
        State { name: "start"; when: statenum == 1
            PropertyChanges { target: animatedrect; color: "blue"; y: firstY }
            PropertyChanges { target: sequentialanimationelementtest
                testtext: "This square will be animated in a sequence.\n"+
                "The next step will see it move quickly down the display, then slowly change its color to green";
            }
        },
        State { name: "firstchange"; when: statenum == 2
            StateChangeScript { script: { firstduration = 1000; secondduration = 3000; startanimationtimer.start() } }
            PropertyChanges { target: sequentialanimationelementtest
                testtext: "The square should have moved quickly and then recolored slowly\n"+
                "Next, it will move slowly and recolor to blue, in sequence."
            }
        },
        State { name: "secondchange"; when: statenum == 3
            StateChangeScript { script: { firstduration = 3000; secondduration = 1000; startanimationtimer.start() } }
            PropertyChanges { target: movement; to: firstY }
            PropertyChanges { target: recolor; to: "blue" }
            PropertyChanges { target: sequentialanimationelementtest
                testtext: "The square should have moved slowly and then recolored quickly\n"+
                "Advance to restart the test"
            }
        }
    ]
}
