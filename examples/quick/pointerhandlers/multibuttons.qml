// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Particles
import QtQuick.Layouts
import "components"

Item {
    width: 800
    height: 800
    ColumnLayout {
        anchors.right: parent.right
        anchors.margins: 20
        spacing: 20
        Text { text: "protagonist"; font.pointSize: 12; font.weight: Font.Bold; bottomPadding: -10 }
        Button {
            id: balloonsButton
            text: "Launch Balloons"
            Layout.fillWidth: true
            gesturePolicy: TapHandler.WithinBounds
            Text {
                anchors { top: parent.bottom; horizontalCenter: parent.horizontalCenter }
                text: "gesturePolicy: WithinBounds"
            }
        }
        Text { text: "the goons"; font.pointSize: 12; font.weight: Font.Bold; bottomPadding: -10 }
        Button {
            id: missilesButton
            text: "Launch Missile"
            Layout.fillWidth: true
            gesturePolicy: TapHandler.ReleaseWithinBounds
            exclusiveSignals: TapHandler.SingleTap
            onTapped: missileEmitter.burst(1)
            Text {
                anchors { top: parent.bottom; horizontalCenter: parent.horizontalCenter }
                text: "gesturePolicy: ReleaseWithinBounds"
            }
        }
        Button {
            id: fightersButton
            text: "Launch Fighters"
            Layout.fillWidth: true
            gesturePolicy: TapHandler.DragThreshold
            Text {
                anchors { top: parent.bottom; horizontalCenter: parent.horizontalCenter }
                text: "gesturePolicy: DragThreshold"
            }
        }
    }
    ParticleSystem {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.leftMargin: 150
        ImageParticle { source: "images/balloon.png" }
        Emitter { anchors.bottom: parent.bottom; enabled: balloonsButton.pressed; lifeSpan: 5000; size: 64
            maximumEmitted: 99
            emitRate: 50; velocity: PointDirection { x: 10; y: -150; yVariation: 30; xVariation: 50 } } }
    ParticleSystem {
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        ImageParticle { source: "images/fighter.png" }
        Emitter { anchors.bottom: parent.bottom; enabled: fightersButton.pressed; lifeSpan: 15000; size: 204
            emitRate: 3; velocity: PointDirection { x: -1000; y: -250; yVariation: 150; xVariation: 50 } } }
    ParticleSystem {
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.rightMargin: 100
        ImageParticle { source: "images/missile.png"; autoRotation: true; rotation: 90 }
        Emitter { id: missileEmitter; anchors.bottom: parent.bottom; lifeSpan: 5000; size: 128;
            emitRate: 0; velocity: PointDirection { x: -200; y: -350; yVariation: 200; xVariation: 100 } } }
}
