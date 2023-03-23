// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Particles

pragma ComponentBehavior: Bound

Rectangle {
    id: root
    color: "black"
    width: 640
    height: 480
    ParticleSystem {
        id: sys
    }
    ImageParticle {
        system: sys
        source: "qrc:///particleresources/glowdot.png"
        color: "white"
        colorVariation: 1.0
        alpha: 0.1
    }

    Component {
        id: emitterComp
        Emitter {
            id: container
            Emitter {
                id: emitMore
                system: sys
                emitRate: 128
                lifeSpan: 600
                size: 16
                endSize: 8
                velocity: AngleDirection {
                    angleVariation: 360
                    magnitude: 60
                }
            }

            property int life: 2600
            property real targetX: 0
            property real targetY: 0
            function go() {
                xAnim.start()
                yAnim.start()
                container.enabled = true
            }
            system: sys
            emitRate: 32
            lifeSpan: 600
            size: 24
            endSize: 8
            NumberAnimation on x {
                id: xAnim;
                to: container.targetX
                duration: container.life
                running: false
            }
            NumberAnimation on y {
                id: yAnim;
                to: container.targetY
                duration: container.life
                running: false
            }
            Timer {
                interval: container.life
                running: true
                onTriggered: container.destroy()
            }
        }
    }

    function customEmit(x, y) {
        //! [0]
        for (var i = 0; i < 8; i++) {
            let obj = emitterComp.createObject(root)
            obj.x = x
            obj.y = y
            obj.targetX = Math.random() * 240 - 120 + obj.x
            obj.targetY = Math.random() * 240 - 120 + obj.y
            obj.life = Math.round(Math.random() * 2400) + 200
            obj.emitRate = Math.round(Math.random() * 32) + 32
            obj.go()
        }
        //! [0]
    }

    Timer {
        interval: 10000
        triggeredOnStart: true
        running: true
        repeat: true
        onTriggered: root.customEmit(Math.random() * 320, Math.random() * 480)
    }
    TapHandler {
        onTapped: function(event) { root.customEmit(event.pressPosition.x, event.pressPosition.y) }
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        text: qsTr("Click Somewhere")
        color: "white"
        font.pixelSize: 24
    }
}
