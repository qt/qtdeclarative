// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
        }

        Emitter{
            //100,100 rect at 100,100
            anchors.fill: parent
            shape: MaskShape{source: "smallmask.png"}
            size: 32
            emitRate: 1000
            lifeSpan: 500
        }
    }
}
