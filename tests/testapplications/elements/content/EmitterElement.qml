// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QtQuick.Particles 2.0

Item {
    id: emitterelementtest
    anchors.fill: parent
    property string testtext: ""

    ParticleSystem {
        id: particlesystem
        anchors.fill: parent
        ImageParticle {
            id: imageparticle
            source: "pics/smile.png"
            color: "red"
            Behavior on color { ColorAnimation { duration: 3000 } }
        }
        Emitter {
            id: emitterelement
            anchors.centerIn: parent
            property int emitvelocity: 10
            emitRate: 5
            lifeSpan: 1000
            velocity: AngleDirection { angle: 0; angleVariation: 360; magnitude: emitterelement.emitvelocity }
        }
    }


    SystemTestHelp { id: helpbubble; visible: statenum != 0
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter; topMargin: 50 }
    }
    BugPanel { id: bugpanel }

    states: [
        State { name: "start"; when: statenum == 1
            PropertyChanges { target: emitterelementtest
                testtext: "This is an Emitter element, visualized by an ImageParticle. It should be emitting particles "+
                "slowly from the center of the display.\n"+
                "Next, let's change the emission velocity of the particles." }
        },
        State { name: "fast"; when: statenum == 2
            PropertyChanges { target: emitterelement; emitvelocity: 50 }
            PropertyChanges { target: emitterelementtest
                testtext: "The particles emitted should be moving more quickly.\n"+
                "Next, let's increase the number of particles emitted." }
        },
        State { name: "many"; when: statenum == 3
            PropertyChanges { target: emitterelement; emitvelocity: 50; emitRate: 100 }
            PropertyChanges { target: emitterelementtest
            testtext: "The particles should now be quick and numerous.\n"+
                "Next, let's allow them to survive longer." }
        },
        State { name: "enduring"; when: statenum == 4
            PropertyChanges { target: emitterelement; emitvelocity: 50; emitRate: 100; lifeSpan: 3000 }
            PropertyChanges { target: emitterelementtest
                testtext: "The particles should now be enduring to the edges of the display.\n"+
                "Next, let's have them changing their size." }
        },
        State { name: "sized"; when: statenum == 5
            PropertyChanges { target: emitterelement; emitvelocity: 50; emitRate: 100; lifeSpan: 3000; size: 20; endSize: 5 }
            PropertyChanges { target: emitterelementtest
                testtext: "The particles should now be starting large and ending small.\n"+
                "Advance to restart the test." }
        }
    ]
}
