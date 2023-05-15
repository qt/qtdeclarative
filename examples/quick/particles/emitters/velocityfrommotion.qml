// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Particles

Rectangle {

    id: root

    height: 540
    width: 360

    gradient: Gradient {
        GradientStop { position: 0; color: "#000020" }
        GradientStop { position: 1; color: "#000000" }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: root
    }

    ParticleSystem { id: sys1 }
    ImageParticle {
        system: sys1
        source: "qrc:///particleresources/glowdot.png"
        alpha: 0
        SequentialAnimation on color {
            loops: Animation.Infinite
            ColorAnimation {
                from: "cyan"
                to: "magenta"
                duration: 1000
            }
            ColorAnimation {
                from: "magenta"
                to: "blue"
                duration: 2000
            }
            ColorAnimation {
                from: "blue"
                to: "violet"
                duration: 2000
            }
            ColorAnimation {
                from: "violet"
                to: "cyan"
                duration: 2000
            }
        }
        colorVariation: 0.3
    }
    //! [0]
    Emitter {
        id: trailsNormal
        system: sys1

        emitRate: 500
        lifeSpan: 2000

        y: mouseArea.pressed ? mouseArea.mouseY : circle.cy
        x: mouseArea.pressed ? mouseArea.mouseX : circle.cx

        velocity: PointDirection {xVariation: 4; yVariation: 4;}
        acceleration: PointDirection {xVariation: 10; yVariation: 10;}
        velocityFromMovement: 8

        size: 8
        sizeVariation: 4
    }
    //! [0]
    ParticleSystem { id: sys2 }
    ImageParticle {
        system: sys2
        alpha: 0
        SequentialAnimation on color {
            loops: Animation.Infinite
            ColorAnimation {
                from: "magenta"
                to: "cyan"
                duration: 1000
            }
            ColorAnimation {
                from: "cyan"
                to: "magenta"
                duration: 2000
            }
        }
        colorVariation: 0.5
        source: "qrc:///particleresources/star.png"
    }
    Emitter {
        id: trailsStars
        system: sys2

        emitRate: 100
        lifeSpan: 2200


        y: mouseArea.pressed ? mouseArea.mouseY : circle.cy
        x: mouseArea.pressed ? mouseArea.mouseX : circle.cx

        velocity: PointDirection {xVariation: 4; yVariation: 4;}
        acceleration: PointDirection {xVariation: 10; yVariation: 10;}
        velocityFromMovement: 8

        size: 22
        sizeVariation: 4
    }
    ParticleSystem { id: sys3; }
    ImageParticle {
        source: "qrc:///particleresources/glowdot.png"
        system: sys3
        alpha: 0
        SequentialAnimation on color {
            loops: Animation.Infinite
            ColorAnimation {
                from: "red"
                to: "green"
                duration: 2000
            }
            ColorAnimation {
                from: "green"
                to: "red"
                duration: 2000
            }
        }

        colorVariation: 0.2

    }
    Emitter {
        id: trailsNormal2
        system: sys3

        emitRate: 300
        lifeSpan: 2000

        y: mouseArea.pressed ? mouseArea.mouseY : circle2.cy
        x: mouseArea.pressed ? mouseArea.mouseX : circle2.cx

        velocityFromMovement: 16

        velocity: PointDirection {xVariation: 4; yVariation: 4;}
        acceleration: PointDirection {xVariation: 10; yVariation: 10;}

        size: 12
        sizeVariation: 4
    }
    ParticleSystem { id: sys4; }
    ImageParticle {
        system: sys4
        source: "qrc:///particleresources/star.png"
        alpha: 0
        SequentialAnimation on color {
            loops: Animation.Infinite
            ColorAnimation {
                from: "green"
                to: "red"
                duration: 2000
            }
            ColorAnimation {
                from: "red"
                to: "green"
                duration: 2000
            }
        }

        colorVariation: 0.5
    }
    Emitter {
        id: trailsStars2
        system: sys4

        emitRate: 50
        lifeSpan: 2200


        y: mouseArea.pressed ? mouseArea.mouseY : circle2.cy
        x: mouseArea.pressed ? mouseArea.mouseX : circle2.cx

        velocityFromMovement: 16
        velocity: PointDirection {xVariation: 2; yVariation: 2;}
        acceleration: PointDirection {xVariation: 10; yVariation: 10;}

        size: 22
        sizeVariation: 4
    }



    color: "white"

    Item {
        id: circle
        //anchors.fill: parent
        property real radius
        property real dx: root.width / 2
        property real dy: root.height / 2
        property real cx: radius * Math.sin(percent*6.283185307179) + dx
        property real cy: radius * Math.cos(percent*6.283185307179) + dy
        property real percent

        SequentialAnimation on percent {
            loops: Animation.Infinite
            running: true
            NumberAnimation {
            duration: 1000
            from: 1
            to: 0
            loops: 8
            }
            NumberAnimation {
            duration: 1000
            from: 0
            to: 1
            loops: 8
            }

        }

        SequentialAnimation on radius {
            loops: Animation.Infinite
            running: true
            NumberAnimation {
                duration: 4000
                from: 0
                to: 100
            }
            NumberAnimation {
                duration: 4000
                from: 100
                to: 0
            }
        }
    }

    Item {
        id: circle3
        property real radius: 100
        property real dx: root.width / 2
        property real dy: root.height / 2
        property real cx: radius * Math.sin(percent*6.283185307179) + dx
        property real cy: radius * Math.cos(percent*6.283185307179) + dy
        property real percent

        SequentialAnimation on percent {
            loops: Animation.Infinite
            running: true
            NumberAnimation { from: 0.0; to: 1 ; duration: 10000;  }
        }
    }

    Item {
        id: circle2
        property real radius: 30
        property real dx: circle3.cx
        property real dy: circle3.cy
        property real cx: radius * Math.sin(percent*6.283185307179) + dx
        property real cy: radius * Math.cos(percent*6.283185307179) + dy
        property real percent

        SequentialAnimation on percent {
            loops: Animation.Infinite
            running: true
            NumberAnimation { from: 0.0; to: 1 ; duration: 1000; }
        }
    }

}
