// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import shared

Item {
    height: 480
    width: 320
    LauncherList {
        id: ll
        anchors.fill: parent
        Component.onCompleted: {
            addExample("Velocity from Motion", "Particle motion just by moving emitters",  Qt.resolvedUrl("velocityfrommotion.qml"));
            addExample("Burst and Pulse", "Emit imperatively",  Qt.resolvedUrl("burstandpulse.qml"));
            addExample("Custom Emitter", "Custom starting state",  Qt.resolvedUrl("customemitter.qml"));
            addExample("Emit Mask", "Emit arbitrary shapes",  Qt.resolvedUrl("emitmask.qml"));
            addExample("Maximum Emitted", "Put a limit on emissions",  Qt.resolvedUrl("maximumemitted.qml"));
            addExample("Shape and Direction", "Creates a portal effect",  Qt.resolvedUrl("shapeanddirection.qml"));
            addExample("TrailEmitter", "Emit from other particles",  Qt.resolvedUrl("trailemitter.qml"));
        }
    }
}
