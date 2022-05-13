// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QtQuick.Particles 2.0

Item {
    id: shapeelementtest
    anchors.fill: parent
    property string testtext: ""

    ParticleSystem {
        id: particlesystem
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        height: 300
        width: 300

        ImageParticle {
            id: imageparticle
            source: "pics/star.png"
            color: "red"
        }

        Emitter {
            id: emitter
            property real magn: 0.1
            anchors.fill: parent
            emitRate: 500
            lifeSpan: 2000
            velocity: TargetDirection {
                targetX: particlesystem.width/2
                targetY: particlesystem.height/2
                proportionalMagnitude: true
                magnitude: emitter.magn
                magnitudeVariation: emitter.magn
            }
            shape: rectangleshapeelement
        }
        Emitter {
            id: emitter2
            enabled: false
            anchors.fill: parent
            emitRate: 200
            lifeSpan: 1000
            velocity: TargetDirection {
                targetX: particlesystem.width/2
                targetY: particlesystem.height/2
                proportionalMagnitude: true
                magnitude: 0
                magnitudeVariation: 0
            }
            shape: lineshapeelement2
        }
        // Shapes
        EllipseShape {
            id: ellipseshapeelement
            fill: false
        }
        RectangleShape {
            id: rectangleshapeelement
            fill: false
        }
        LineShape {
            id: lineshapeelement
        }
        LineShape {
            id: lineshapeelement2
            mirrored: true
        }
        MaskShape {
            id: maskshapeelement
            source: "pics/logo-hollowed.png"
        }
    }


    SystemTestHelp { id: helpbubble; visible: statenum != 0
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter; topMargin: 50 }
    }
    BugPanel { id: bugpanel }

    states: [
        State { name: "start"; when: statenum == 1
            PropertyChanges { target: shapeelementtest
                testtext: "This is an Shape element, used by Emitter. There should be a rectangle "+
                "shape emitting into its center.\n"+
                "Next, let's change the shape to an ellipse." }
        },
        State { name: "ellipse"; when: statenum == 2
            PropertyChanges { target: emitter; shape: ellipseshapeelement }
            PropertyChanges { target: shapeelementtest
                testtext: "The particles should now be emitted in a circular shape.\n"+
                "Next, let's change the shape to a line." }
        },
        State { name: "line"; when: statenum == 3
            PropertyChanges { target: emitter; shape: lineshapeelement; lifeSpan: 1000; emitRate: 200; }
            PropertyChanges { target: emitter2; enabled: true }
            PropertyChanges { target: shapeelementtest
            testtext: "The particles should now be emitted from two lines, creating an X shape.\n"+
                "Next, let's change the shape to an image." }
        },
        State { name: "enduring"; when: statenum == 4
            PropertyChanges { target: emitter; shape: maskshapeelement; lifeSpan: 1000; emitRate: 1000; magn: 0 }
            PropertyChanges { target: shapeelementtest
                testtext: "The particles should now be sparkling, stationary within a 'Qt' text image.\n"+
                "Advance to restart the test." }
        }
    ]
}
