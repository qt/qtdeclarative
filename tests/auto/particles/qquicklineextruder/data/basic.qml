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
            groups: ["", "notdefault"]
            source: "../../shared/star.png"
        }

        Emitter{
            anchors.fill: parent
            shape: LineShape{}
            size: 32
            emitRate: 1000
            lifeSpan: 500
        }
        Emitter{
            anchors.fill: parent
            group: "notdefault"
            shape: LineShape{mirrored: true}
            size: 32
            emitRate: 1000
            lifeSpan: 500
        }
    }
}
