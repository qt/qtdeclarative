/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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

Rectangle{
    width: 360
    height: 600
    color: "black"
    Component{
        id: firework
        Item{
            id: container
            width: 48
            height: 48
            Image{
                width: 48
                height: 48
                id: img
                source: "content/particle.png"
            }
            Timer{
                interval: 1000 + 4000*Math.random()
                running: true
                repeat: false
                onTriggered: {
                    img.visible = false;
                    emitter.burst(100);
                }
            }
            Emitter{
                anchors.centerIn: parent
                id: emitter
                system: syssy
                particle: "works"
                emitting: false
                emitRate: 100
                lifeSpan: 1000
                //speed: AngledDirection{angle: 270; angleVariation:60; magnitudeVariation: 60; magnitude: 20}
                speed: PointDirection{y:-60; yVariation: 80; xVariation: 80}
                acceleration: PointDirection{y:100; yVariation: 20}
            }
        }
    }
    ParticleSystem{
        anchors.fill: parent
        id: syssy
        Emitter{
            particle: "fire"
            width: parent.width
            y: parent.height
            emitRate: 2
            lifeSpan: 6000
            speed: PointDirection{y:-100}
        }
        ItemParticle{
            particles: ["fire"]
            delegate: firework
        }
        ImageParticle{
            particles: ["works"]
            source: "content/particle.png"
        }
    }
}

