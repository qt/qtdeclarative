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
        property real resultX1: 1234
        property real resultY1: 1234
        property real resultX2: 1234
        property real resultY2: 1234

        ImageParticle {
            source: "../../shared/star.png"
            rotation: 90
        }

        Emitter{
            //0,100 position
            y: 100
            size: 32
            emitRate: 1000
            lifeSpan: 500
        }

        Affector {
            once: true
            onAffected: {//Does nothing else, so should be called for all particles
                sys.resultX1 = x;
                sys.resultY1 = y;
            }
        }

        Affector {
            once: true
            relative: false
            position: PointDirection { x: 0; y: 100; }
            onAffected: {//Does something, so should only be called when it causes a change (it won't)
                sys.resultX2 = x;
                sys.resultY2 = y;
            }
        }
    }
}
