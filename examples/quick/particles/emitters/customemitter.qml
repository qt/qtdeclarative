// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Particles

ParticleSystem {
    id: sys
    width: 360
    height: 600
    running: true
    Rectangle {
        z: -1
        anchors.fill: parent
        color: "black"
    }

    property real petalLength: 180
    property real petalRotation
    NumberAnimation on petalRotation {
        from: 0;
        to: 360;
        loops: -1;
        running: true
        duration: 24000
    }

    function convert(a) {return a*(Math.PI/180);}
    Emitter {
        lifeSpan: 4000
        emitRate: 120
        size: 12
        anchors.centerIn: parent
        //! [0]
        onEmitParticles: (particles) => {
            for (var i=0; i<particles.length; i++) {
                let particle = particles[i];
                particle.startSize = Math.max(02,Math.min(492,Math.tan(particle.t/2)*24));
                let theta = Math.floor(Math.random() * 6.0);
                particle.red = theta == 0 || theta == 1 || theta == 2 ? 0.2 : 1;
                particle.green = theta == 2 || theta == 3 || theta == 4 ? 0.2 : 1;
                particle.blue = theta == 4 || theta == 5 || theta == 0 ? 0.2 : 1;
                theta /= 6.0;
                theta *= 2.0*Math.PI;
                theta += sys.convert(sys.petalRotation);//Convert from degrees to radians
                particle.initialVX = sys.petalLength * Math.cos(theta);
                particle.initialVY = sys.petalLength * Math.sin(theta);
                particle.initialAX = particle.initialVX * -0.5;
                particle.initialAY = particle.initialVY * -0.5;
            }
        }
        //! [0]
    }

    ImageParticle {
        source: "qrc:///particleresources/fuzzydot.png"
        alpha: 0.0
    }
}
