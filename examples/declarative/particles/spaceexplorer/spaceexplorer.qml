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
import "content/helpers.js" as Helpers

Rectangle{
    id: root
    width: 360
    height: 540
    color: "black"
    Image{
        anchors.centerIn: parent
        source: "../asteroid/content/finalfrontier.png"
    }
    property bool spacePressed: false
    property int holeSize: 4
    focus: true
    Keys.onPressed: {
        if (event.key == Qt.Key_Space) {
            spacePressed = true;
            event.accepted = true;
        }
    }
    Keys.onReleased: {
        if (event.key == Qt.Key_Space) {
            spacePressed = false;
            event.accepted = true;
        }
    }

    function fakeMove(){
        fakeMoving = rocket.x < 80 || rocket.x+rocket.width-root.width > -80 || rocket.y < 80 || rocket.y+rocket.height-root.height > -80;
        if(fakeMoving)
            fakeMovementDir = Helpers.direction(root.width/2, root.height/2, rocket.x, rocket.y) + 180;
    }
    property bool fakeMoving: false
    property real fakeMovementDir: 0

    Emitter{
        particle: "stars2"
        system: background
        emitRate: 60
        lifeSpan: 4000
        emitting: true
        size: 10
        sizeVariation: 10
        anchors.fill: parent
    }
    ParticleSystem{ id: background }
    ImageParticle{
        particles: ["stars2"]
        system: background
        anchors.fill: parent
        source: "content/star.png"
        color: "white"
        colorVariation: 0.1
    }
    Gravity{
        system: background
        anchors.fill: parent
        acceleration: fakeMoving?10:0
        angle: fakeMovementDir
    }
    Text{
        color: "white"
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        text:"Drag the ship, but don't hit a black hole!"
        font.pixelSize: 10
    }
    Text{
        color: "white"
        font.pixelSize: 36
        anchors.centerIn: parent
        text: "GAME OVER"
        opacity: gameOver ? 1 : 0
        Behavior on opacity{NumberAnimation{}}
    }
    Text{
        color: "white"
        font.pixelSize: 18
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 8
        text: "Score: " + score
    }
    Image{
        source: "content/star.png"
        width: 40
        height: 40
        anchors.right: parent.right
        anchors.top: parent.top
        MouseArea{
            anchors.fill: parent
            anchors.margins: -20
            onClicked: shoot = !shoot
        }
    }
    property int score: 0
    property bool gameOver: false
    property bool shoot: true
    property int maxLives: 3
    property int lives: maxLives
    property bool alive: !Helpers.intersects(rocket, gs1.x, gs1.y, holeSize) && !Helpers.intersects(rocket, gs2.x, gs2.y, holeSize) && !Helpers.intersects(rocket, gs3.x, gs3.y, holeSize)  && !Helpers.intersects(rocket, gs4.x, gs4.y, holeSize);
    onAliveChanged: if(!alive){
        lives -= 1;
        if(lives == -1){
            console.log("game over");
            gameOver = true;
        }
    }
    Row{
        Repeater{
            model: maxLives
            delegate: Image{ 
                opacity: index < lives ? 1 : 0
                Behavior on opacity{NumberAnimation{}}
                source: "content/rocket.png" 
            }
        }
    }

    property real courseDur: 10000
    property real vorteX: width/4
    property real vorteY: height/4
    Behavior on vorteX{NumberAnimation{duration: courseDur}}
    Behavior on vorteY{NumberAnimation{duration: courseDur}}
    property real vorteX2: width/4
    property real vorteY2: 3*height/4
    Behavior on vorteX2{NumberAnimation{duration: courseDur}}
    Behavior on vorteY2{NumberAnimation{duration: courseDur}}
    property real vorteX3: 3*width/4
    property real vorteY3: height/4
    Behavior on vorteX3{NumberAnimation{duration: courseDur}}
    Behavior on vorteY3{NumberAnimation{duration: courseDur}}
    property real vorteX4: 3*width/4
    property real vorteY4: 3*height/4
    Behavior on vorteX4{NumberAnimation{duration: courseDur}}
    Behavior on vorteY4{NumberAnimation{duration: courseDur}}
    Timer{
        id: vorTimer
        interval: courseDur
        running: true
        repeat: true
        triggeredOnStart: true
        onTriggered: {
            vorteX = Math.random() * width * 2 - width * 0.5;
            vorteY = Math.random() * height * 2 - height * 0.5;
            vorteX2 = Math.random() * width * 2 - width * 0.5;
            vorteY2 = Math.random() * height * 2 - height * 0.5;
            vorteX3 = Math.random() * width * 2 - width * 0.5;
            vorteY3 = Math.random() * height * 2 - height * 0.5;
            vorteX4 = Math.random() * width * 2 - width * 0.5;
            vorteY4 = Math.random() * height * 2 - height * 0.5;
        }
    }



    ParticleSystem{ id: foreground }
    ImageParticle{
        particles: ["stars"]
        anchors.fill: parent
        system: foreground
        source: "content/star.png"
        color: "white"
        colorVariation: 0.1
    }
    ImageParticle{
        particles: ["shot"]
        anchors.fill: parent
        system: foreground
        source: "content/star.png"

        color: "orange"
        colorVariation: 0.3
    }
    ImageParticle{
        id: engine
        particles: ["engine"]
        anchors.fill: parent
        system: foreground
        source: "content/particle4.png"

        color: "orange"
        SequentialAnimation on color {
            loops: Animation.Infinite
            ColorAnimation {
                from: "red"
                to: "cyan"
                duration: 1000
            }
            ColorAnimation {
                from: "cyan"
                to: "red"
                duration: 1000
            }
        }

        colorVariation: 0.2
    }
    ImageParticle{
        particles: ["powerups"]
        anchors.fill: parent
        system: foreground
        sprites:[Sprite{
            name: "norm"
            source: "content/powerupScore.png"
            frames: 35
            duration: 40
            to: {"norm":1, "got":0}
        },
        Sprite{
            name: "got"
            source: "content/powerupScore_got.png"
            frames: 22
            duration: 40
            to: {"null":1}
        },
        Sprite{
            name: "null"
            source: "content/powerupScore_gone.png"
            frames: 1
            duration: 1000
        }
        ]
    }
    SpriteGoal{
        x: rocket.x - 30
        y: rocket.y - 30
        width: 60
        height: 60
        goalState: "got"
        jump: true
        onAffected: if(!gameOver) score += 1000
        system: foreground
    }
    PointAttractor{
        proportionalToDistance: PointAttractor.InverseQuadratic;
        id: gs1; pointX: vorteX; pointY: vorteY; strength: 800000;
        system: foreground
    }
    Kill{
        x: gs1.pointX - holeSize;
        y: gs1.pointY - holeSize;
        width: holeSize * 2
        height: holeSize * 2
        system: foreground
    }

    PointAttractor{
        proportionalToDistance: PointAttractor.InverseQuadratic;
        id: gs2; pointX: vorteX2; pointY: vorteY2; strength: 800000;
        system: foreground
    }
    Kill{
        x: gs2.pointX - holeSize;
        y: gs2.pointY - holeSize;
        width: holeSize * 2
        height: holeSize * 2
        system: foreground
    }

    PointAttractor{
        proportionalToDistance: PointAttractor.InverseQuadratic;
        id: gs3; pointX: vorteX3; pointY: vorteY3; strength: 800000;
        system: foreground
    }
    Kill{
        x: gs3.pointX - holeSize;
        y: gs3.pointY - holeSize;
        width: holeSize * 2
        height: holeSize * 2
        system: foreground
    }
    PointAttractor{
        id: gs4; pointX: vorteX4; pointY: vorteY4; strength: 800000;
        proportionalToDistance: PointAttractor.InverseQuadratic;
        system: foreground
    }
    Kill{
        x: gs4.pointX - holeSize;
        y: gs4.pointY - holeSize;
        width: holeSize * 2
        height: holeSize * 2
        system: foreground
    }        
    Emitter{
        particle: "powerups"
        system: foreground
        emitRate: 1
        lifeSpan: 6000
        emitting: !gameOver
        size: 60
        sizeVariation: 10
        anchors.fill: parent
    }
    Emitter{
        particle: "stars"
        system: foreground
        emitRate: 40
        lifeSpan: 4000
        emitting: !gameOver
        size: 30
        sizeVariation: 10
        anchors.fill: parent
    }
    SpriteImage{
        id: rocket
        //Sprites or children for default?
        Sprite{
            name: "normal"
            source: "content/rocket2.png"
            frames: 1
            duration: 1000
            to: {"normal": 0.9, "winking" : 0.1}
        }
        Sprite{
            name: "winking"
            source: "content/rocketEye.png"
            frames: 10
            duration: 40
                to: {"normal" : 1}
        }
        x: root.width/2
        y: root.height/2
        property int lx: 0
        property int ly: 0
        property int lastX: 0
        property int lastY: 0
        width: 45
        height: 22
        onXChanged:{ lastX = lx; lx = x; fakeMove()}
        onYChanged:{ lastY = ly; ly = y; fakeMove()}
        rotation: Helpers.direction(lastX, lastY, x, y)
        data:[
        MouseArea{
            id: ma
            anchors.fill: parent;
            drag.axis: Drag.XandYAxis
            drag.target: rocket
        },
        Emitter{
            system: foreground
            particle: "engine"
            emitRate: 100
            lifeSpan: 1000
            emitting: !gameOver 
            size: 10
            endSize: 4
            sizeVariation: 4
            speed: PointDirection{
                x: -128 * Math.cos(rocket.rotation * (Math.PI / 180))
                y: -128 * Math.sin(rocket.rotation * (Math.PI / 180))
            }
            anchors.verticalCenter: parent.verticalCenter
            height: 4
            width: 4
            
        }, 
        Emitter{
            system: foreground
            particle: "shot"
            emitRate: 16
            lifeSpan: 1600
            emitting: !gameOver && shoot
            size: 40
            speed: PointDirection{
                x: 256 * Math.cos(rocket.rotation * (Math.PI / 180))
                y: 256 * Math.sin(rocket.rotation * (Math.PI / 180))
            }
            x: parent.width - 4
            y: parent.height/2
        }
        ]
    }
}

