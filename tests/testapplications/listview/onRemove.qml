// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtQuick.Particles 2.0
Item {
    id: root
    width: 450; height: 600

    Component {
        id: viewDelegate

        Rectangle {
            id: item
            signal boom
            Connections {
                target: item
                onBoom: emitter.burst(1000)
            }

            width: 225; height: 40
            border.width: ListView.isCurrentItem ? 3 : 1
            z: ListView.isCurrentItem ? 100 : 1
            color: original ? "lightsteelblue" : "yellow"
            objectName: name
            Text { x: 10; text: name; font.pixelSize: 20 }
            MouseArea { anchors.fill: parent; onClicked: listview.currentIndex = index }

            Emitter {
                id: emitter
                system: ps
                anchors.fill: parent
                enabled: false
                velocity: AngleDirection {
                    angle: 0
                    angleVariation: 360
                    magnitude: 50
                    magnitudeVariation: 50
                }
                lifeSpan: 2000
            }

            SequentialAnimation {
                id: removeAnimation
                PropertyAction { target: item; property: "ListView.delayRemove"; value: true }
                PropertyAction { target: item; property: "opacity"; value: 0 }
                ScriptAction { script: item.boom() }
                PauseAnimation { duration: 1000 }
                PropertyAction { target: item; property: "ListView.delayRemove"; value: false }
            }
            ListView.onRemove: removeAnimation.start()
        }
    }


    ListView {
        id: listview
        width: 225; height: 500
        anchors.centerIn: parent
        delegate: viewDelegate
        header: Rectangle {
            height: 50; width: 225
            color: "blue"
            Text { anchors.centerIn: parent; text: "Transitions!"; color: "goldenrod" }
        }
        model: ListModel {
            id: a_model
            ListElement { name: "Item A"; original: true }
            ListElement { name: "Item B"; original: true }
            ListElement { name: "Item C"; original: true }
            ListElement { name: "Item D"; original: true }
            ListElement { name: "Item E"; original: true }
            ListElement { name: "Item F"; original: true }
        }
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.color: "black"
        }

    }

    ParticleSystem {
        id: ps
        ImageParticle {
            id: imageparticle
            source: "star.png"
            color: "blue"
        }
    }
    Column {
        spacing: 2
        Rectangle {
            gradient: Gradient {
                GradientStop { position: 0.0; color: "darkgray" }
                GradientStop { position: 0.5; color: "lightgray" }
                GradientStop { position: 1.0; color: "darkgray" }
            }
            radius: 6
            border.color: "black"
            height: 50; width: 80
            Text { anchors.centerIn: parent; text: "+"; font.pixelSize: 25; font.bold: true }
            MouseArea { anchors.fill: parent; onClicked: listview.model.insert(listview.currentIndex+1, {"name": "New item", "original": false } ) }
        }
        Rectangle {
            gradient: Gradient {
                GradientStop { position: 0.0; color: "darkgray" }
                GradientStop { position: 0.5; color: "lightgray" }
                GradientStop { position: 1.0; color: "darkgray" }
            }
            radius: 6
            border.color: "black"
            height: 50; width: 80
            Text { anchors.centerIn: parent; text: "-"; font.pixelSize: 25; font.bold: true }
            MouseArea { anchors.fill: parent; onClicked: listview.model.remove(listview.currentIndex) }
        }
    }
}



