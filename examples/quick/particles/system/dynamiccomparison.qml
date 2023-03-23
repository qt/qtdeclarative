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
        entryEffect: ImageParticle.None
    }

    Emitter {
        id: emitter
        system: sys
        width: parent.width/2
        velocity: PointDirection {y: 72; yVariation: 24}
        lifeSpan: 10000
        emitRate: 1000
        enabled: false
        size: 32
    }

    //! [fake]
    Item {
        id: fakeEmitter
        function burst(number) {
            while (number > 0) {
                let item = fakeParticle.createObject(root)
                item.lifeSpan = Math.random() * 5000 + 5000
                item.x = Math.random() * (root.width / 2) + (root.width / 2)
                item.y = 0
                number--
            }
        }

        Component {
            id: fakeParticle
            Image {
                id: container
                property int lifeSpan: 10000
                width: 32
                height: 32
                source: "qrc:///particleresources/glowdot.png"
                PropertyAnimation on y { from: -16; to: root.height - 16; duration: container.lifeSpan; running: true }
                SequentialAnimation on opacity {
                    running: true
                    NumberAnimation { from: 0; to: 1; duration: 500 }
                    PauseAnimation { duration: container.lifeSpan - 1000 }
                    NumberAnimation { from: 1; to: 0; duration: 500 }
                    ScriptAction { script: container.destroy(); }
                }
            }
        }
    }
    //! [fake]

    //Hooked to a timer, but click for extra bursts that really stress performance
    Timer {
        interval: 10000
        triggeredOnStart: true
        repeat: true
        running: true
        onTriggered: {
            emitter.burst(1000);
            fakeEmitter.burst(1000);
        }
    }
    Text {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        text: qsTr("1000 particles")
        color: "white"
        MouseArea {
            anchors.fill: parent
            onClicked: emitter.burst(1000);
        }
    }
    Text {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        text: qsTr("1000 items")
        color: "white"
        MouseArea {
            anchors.fill: parent
            onClicked: fakeEmitter.burst(1000);
        }
    }
}
