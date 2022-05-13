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
            addExample("Age", "Kills off particles that enter the affector",  Qt.resolvedUrl("age.qml"));
            addExample("Attractor", "Simulates a small black hole", Qt.resolvedUrl("attractor.qml"));
            addExample("Custom Affector", "Custom falling leaves", Qt.resolvedUrl("customaffector.qml"));
            addExample("Friction", "Leaves that slow down as they fall", Qt.resolvedUrl("friction.qml"));
            addExample("Gravity", "Leaves that fall towards the earth as you move it", Qt.resolvedUrl("gravity.qml"));
            addExample("GroupGoal", "Balls that can be set on fire various ways", Qt.resolvedUrl("groupgoal.qml"));
            addExample("Move", "Some effects you can get by altering trajectory midway", Qt.resolvedUrl("move.qml"));
            addExample("SpriteGoal", "A ship that makes asteroids explode", Qt.resolvedUrl("spritegoal.qml"));
            addExample("Turbulence", "A candle with faint wind", Qt.resolvedUrl("turbulence.qml"));
            addExample("Wander", "Drifting snow flakes", Qt.resolvedUrl("wander.qml"));
        }
    }
}
