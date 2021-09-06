/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
