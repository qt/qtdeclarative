/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the examples of the Qt Toolkit.
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
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
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
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtQuick.Particles 2.0

Rectangle {
    width: 360
    height: 600
    color: "#222222"
    id: root
    Image {
        source: "../images/candle.png"
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: -8
        anchors.horizontalCenterOffset: 2
    }
    ParticleSystem {
        id: ps
    }
    MouseArea {
        anchors.fill: parent
        onClicked: turb.enabled = !turb.enabled
    }

    Turbulence {
        id: turb
        system: ps
        enabled: true
        height: (parent.height / 2)
        width: parent.width / 2
        x: parent. width / 4
        anchors.fill: parent
        strength: 32
    }

    ImageParticle {
        groups: ["smoke"]
        system: ps
        source: "../images/particle.png"
        color: "#11111111"
        colorVariation: 0
    }
    ImageParticle {
        groups: ["flame"]
        system: ps
        source: "../images/particle.png"
        color: "#11ff400f"
        colorVariation: 0.1
    }
    Emitter {
        anchors.centerIn: parent
        system: ps
        group: "flame"
        
        emitRate: 120
        lifeSpan: 1200
        size: 20
        endSize: 10
        sizeVariation: 10
        acceleration: PointDirection { y: -40 }
        speed: AngleDirection { angle: 270; magnitude: 20; angleVariation: 22; magnitudeVariation: 5 }
    }
    TrailEmitter {
        id: smoke1
        width: root.width
        height: root.height/2 - 20
        system: ps
        group: "smoke"
        follow: "flame"

        emitRatePerParticle: 1
        lifeSpan: 2400
        lifeSpanVariation: 400
        size: 16
        endSize: 8
        sizeVariation: 8
        acceleration: PointDirection { y: -40 }
        speed: AngleDirection { angle: 270; magnitude: 40; angleVariation: 22; magnitudeVariation: 5 }
    }
    TrailEmitter {
        id: smoke2
        width: root.width
        height: root.height/2 - 40
        system: ps
        group: "smoke"
        follow: "flame"
        
        emitRatePerParticle: 4
        lifeSpan: 2400
        size: 36
        endSize: 24
        sizeVariation: 12
        acceleration: PointDirection { y: -40 }
        speed: AngleDirection { angle: 270; magnitude: 40; angleVariation: 22; magnitudeVariation: 5 }
    }
}
