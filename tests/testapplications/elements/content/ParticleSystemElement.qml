// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QtQuick.Particles 2.0

Item {
    id: particlesystemelementtest
    anchors.fill: parent
    property string testtext: ""
    Rectangle { x: 50; y: 200; height: 300; width: 260; color: "transparent"; border.color: "lightgray" }
    ParticleSystem {
        id: particlesystemelement
        anchors.fill: parent
        ImageParticle { source: "pics/star.png"; color: "red" }
        Emitter {
            id: particleemitter
            x: 50; y: 200
            emitRate: 100
            velocity: AngleDirection { angle: 0; angleVariation: 360; magnitude: 100 }
            Rectangle { anchors.centerIn: parent; height: 1; width: 1; color: "black" }
            SequentialAnimation {
                running: true; paused: particlesystemelement.paused; loops: Animation.Infinite
                NumberAnimation { target: particleemitter; properties: "x"; to: 310; duration: 3000 }
                NumberAnimation { target: particleemitter; properties: "y"; to: 500; duration: 3000 }
                NumberAnimation { target: particleemitter; properties: "x"; to: 50;  duration: 3000 }
                NumberAnimation { target: particleemitter; properties: "y"; to: 200; duration: 3000 }
            }
        }
    }


    SystemTestHelp { id: helpbubble; visible: statenum != 0
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter; topMargin: 50 }
    }
    BugPanel { id: bugpanel }

    states: [
        State { name: "start"; when: statenum == 1
            PropertyChanges { target: particlesystemelementtest
                testtext: "This is an ParticleSystem element. It is represented by an ImageParticle, "+
                "tracing around the display.\n"+
                "Next, it should pause simulation." }
        },
        State { name: "paused"; when: statenum == 2
            PropertyChanges { target: particlesystemelement; paused: true }
            PropertyChanges { target: particlesystemelementtest
                testtext: "The simulation should now be paused.\n"+
                "Advance to resume simulation." }
        },
        State { name: "resumed"; when: statenum == 3
            // PropertyChanges { target: bugpanel; bugnumber: "21539" } FIXED
            PropertyChanges { target: particlesystemelement; paused: false }
            PropertyChanges { target: particlesystemelementtest
                testtext: "The simulation should now be active.\n"+
                "Advance to restart the test" }
        }
    ]
}
