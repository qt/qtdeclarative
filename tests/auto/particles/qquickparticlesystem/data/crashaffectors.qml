// Copyright (C) 2017 reMarkable A/S
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.6
import QtQuick.Particles 2.0

ParticleSystem {
    running: false
    Affector { Component.onCompleted: destroy() }
    Emitter {
        Timer { interval: 1; running: true; onTriggered: parent.lifeSpan = 1 }
    }
}
