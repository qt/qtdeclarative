// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtQuick.Particles 2.0

Item {
    id: trailemitterelementtest
    anchors.fill: parent
    property string testtext: ""

    ParticleSystem {
        id: particlesystem
        anchors.fill: parent
        Image {
            id: backgroundpic
            anchors.fill: parent
            source: "pics/logo.png"
            opacity: 0
            Behavior on opacity { NumberAnimation { duration: 1000 } }
        }
        ImageParticle {
            id: omissile
            source: "pics/star.png"
            color: "orange"
            entryEffect: ImageParticle.None
            groups: ["orangemissile"]
        }
        ImageParticle {
            id: gmissile
            source: "pics/star.png"
            color: "green"
            entryEffect: ImageParticle.None
            groups: ["greenmissile"]
        }
        ImageParticle {
            id: sparks
            source: "pics/star.png"
            color: "red"
            colorVariation: .5
            entryEffect: ImageParticle.None
            groups: ["sparks"]
        }
        Emitter {
            id: emitter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 5
            anchors.horizontalCenter: parent.horizontalCenter
            emitRate: 1
            lifeSpan: 3000
            size: 20
            velocity: AngleDirection { angle: 270; angleVariation: 25; magnitude: 150 }
            group: "orangemissile"
        }
        Emitter {
            id: emitter2
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 5
            anchors.horizontalCenter: parent.horizontalCenter
            emitRate: 1
            lifeSpan: 3000
            size: 20
            velocity: AngleDirection { angle: 270; angleVariation: 25; magnitude: 150 }
            group: "greenmissile"
        }
        Gravity {
            anchors.fill: parent
            angle: 90
            acceleration: 30
        }
        TrailEmitter {
            id: trailemitterelement
            follow: "orangemissile"
            group:"sparks"
            anchors.fill: parent
            emitRatePerParticle: 50
            lifeSpan: 1000
            velocityFromMovement: .2
            velocity: AngleDirection { angle: 0; angleVariation: 360; magnitude: 5 }
            maximumEmitted: 500
            shape: basicshape
        }

        RectangleShape { id: basicshape }

        MaskShape {
            id: maskshape
            source: "pics/logo-hollowed.png"
        }

    }

    SystemTestHelp { id: helpbubble; visible: statenum != 0
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter; topMargin: 50 }
    }
    BugPanel { id: bugpanel }

    states: [
        State { name: "start"; when: statenum == 1
            PropertyChanges { target: trailemitterelementtest
                testtext: "This is a TrailEmitter, with particles following the orange particles.\n"+
                "The green particles should not be followed by other particles.\n"+
                "Next, let's change the sparks to follow the green particles." }
        },
        State { name: "followgreen"; when: statenum == 2
            PropertyChanges { target: trailemitterelement; follow: "greenmissile" }
            PropertyChanges { target: trailemitterelementtest
                testtext: "The particles should be following the green particles.\n"+
                "Next, let's add a shape to emit within." }
        },
        State { name: "onlyinshape"; when: statenum == 3
            PropertyChanges { target: trailemitterelement; follow: "greenmissile"; shape: maskshape }
            PropertyChanges { target: backgroundpic; opacity: .5 }
            PropertyChanges { target: trailemitterelementtest
            testtext: "The particles should now be only emitted when they pass through the 'Qt' text.\n"+
                "Next, let's create small Qt missiles." }
        },
        State { name: "emittingashape"; when: statenum == 4
            PropertyChanges { target: trailemitterelement; follow: "greenmissile"; shape: basicshape
                emitHeight: 60; emitWidth: 60; emitRatePerParticle: 1500; emitShape: maskshape; maximumEmitted: 1500; lifeSpan: 50
            }
            PropertyChanges { target: trailemitterelementtest
                testtext: "The particles should now be Qt text shaped particles.\n"+
                "Advance to restart the test." }
        }
    ]
}
