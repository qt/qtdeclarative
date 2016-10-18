/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the manual tests of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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

import QtQuick 2.0
import QtQuick.Particles 2.0
import QtQuick.Layouts 1.0
import Qt.labs.handlers 1.0
import "content"

Item {
    width: 800
    height: 800
    ColumnLayout {
        anchors.right: parent.right
        spacing: 20
        Text { text: "protagonist"; font.pointSize: 12 }
        MultiButton {
            id: balloonsButton
            label: "Launch Balloons"
            Layout.fillWidth: true
            gesturePolicy: TapHandler.DragThreshold
        }
        Text { text: "the goons"; font.pointSize: 12 }
        MultiButton {
            id: missilesButton
            label: "Launch Missiles"
            Layout.fillWidth: true
            gesturePolicy: TapHandler.WithinBounds
        }
        MultiButton {
            id: fightersButton
            label: "Launch Fighters"
            Layout.fillWidth: true
            gesturePolicy: TapHandler.ReleaseWithinBounds
        }
    }
    ParticleSystem {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.leftMargin: 150
        ImageParticle { source: "resources/balloon.png" }
        Emitter { anchors.bottom: parent.bottom; enabled: balloonsButton.pressed; lifeSpan: 5000; size: 64
            maximumEmitted: 99
            emitRate: 50; velocity: PointDirection { x: 10; y: -150; yVariation: 30; xVariation: 50 } } }
    ParticleSystem {
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        ImageParticle { source: "resources/fighter.png" }
        Emitter { anchors.bottom: parent.bottom; enabled: fightersButton.pressed; lifeSpan: 15000; size: 204
            emitRate: 3; velocity: PointDirection { x: -1000; y: -250; yVariation: 150; xVariation: 50 } } }
    ParticleSystem {
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.rightMargin: 100
        ImageParticle { source: "resources/missile.png"; autoRotation: true; rotation: 90 }
        Emitter { anchors.bottom: parent.bottom; enabled: missilesButton.pressed; lifeSpan: 5000; size: 128
            emitRate: 10; velocity: PointDirection { x: -200; y: -350; yVariation: 200; xVariation: 100 } } }
}
