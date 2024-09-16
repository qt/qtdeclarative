// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtQuick.Particles 2.0

Rectangle {
    color: "black"
    width: 320
    height: 320

    ParticleSystem {
        id: sys
        objectName: "system"
        anchors.fill: parent

        ImageParticle {
            source: "../../shared/star.png"
            rotation: 90
        }

        Emitter{
            //0,0 position
            size: 32
            emitRate: 1000
            lifeSpan: 500
        }

        Affector {
            once: true
            onAffectParticles: {
                for (var i=0; i<particles.length; i++) {
                    particles[i].initialX = 100;
                    particles[i].initialY = 100;
                    particles[i].initialVX = 100;
                    particles[i].initialVY = 100;
                    particles[i].initialAX = 100;
                    particles[i].initialAY = 100;
                    particles[i].startSize = 100;
                    particles[i].endSize = 100;
                    particles[i].autoRotate = true;
                    particles[i].update = true;
                    particles[i].red = 0;
                    particles[i].green = 1.0;
                    particles[i].blue = 0;
                    particles[i].alpha = 0;
                }
            }
        }
    }
}
