// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtQuick.Particles 2.0

Rectangle {
    color: "black"
    width: 320
    height: 320

    Repeater {
        model: 100
        delegate: Image {
            id: img
            Component.onCompleted: {
                sys.acc = sys.acc + 1;
                ip.take(img);
            }
            Component.onDestruction: sys.acc = sys.acc - 1;

            //Test uses the recycling case because it's most realistic
            //Attempts by ItemParticle to delete the delegate should lead to a segfault
            ItemParticle.onDetached: ip.take(img);

            source: "../../shared/star.png"
        }
    }

    ParticleSystem {
        id: sys
        objectName: "system"
        anchors.fill: parent
        property int acc: 0

        ItemParticle {
            id: ip
        }

        Emitter{
            //0,0 position
            size: 32
            emitRate: 1000
            lifeSpan: 100
        }
    }
}
