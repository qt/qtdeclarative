// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Particles
import shared
import "script.js" as Script

Item {
    id: root
    width: 640
    height: 480
    Rectangle {
        anchors.fill: parent
        color: "black"
        z: -1
    }
    Item {
        id: loading
        Behavior on opacity {NumberAnimation {}}
        anchors.fill: parent
        Text {
            anchors.centerIn: parent
            text: "Loading"
            color: "white"
        }
    }
    ParticleSystem {
        id: sys;
        running: true
    }
    Emitter {
        id: emitter
        system: sys
        height: parent.height - 132/2
        x: -132/2
        y: 132/2
        velocity: PointDirection { x: 32; xVariation: 8 }
        emitRate: 0.5
        lifeSpan: Emitter.InfiniteLife
        group: "photos"
    }
    Age {
        system: sys
        x: parent.width + 132/2
        height: parent.height
        width: 1000
    }
    ImageParticle {
        system: sys
        groups: ["fireworks"]
        source: "qrc:///particleresources/star.png"
        color: "lightsteelblue"
        alpha: 0
        colorVariation: 0
        z: 1000
    }
    ItemParticle {
        id: mp
        z: 0
        system: sys
        fade: false
        groups: ["photos"]
    }
    Component {
        id: alertDelegate
        Rectangle {
            color: "DarkSlateGray"
            border.width: 1
            border.color: "LightSteelBlue"
            width: 144
            height: 132
            antialiasing: true
            radius: 3
            NumberAnimation on scale {
                running: true
                loops: 1
                from: 0.2
                to: 1
            }
            Image {
                source: "images/rocket.png"
                anchors.centerIn: parent
            }
            Text {
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                text: "A new ship has arrived!"
                color: "LightSteelBlue"
            }
        }
    }
    property Item alertItem;
    function alert() {
        force.enabled = true;
        alertItem = alertDelegate.createObject(root);
        alertItem.x = root.width/2 - alertItem.width/2
        alertItem.y = root.height/2 - alertItem.height/2
        spawnFireworks.pulse(200);
        stopAlert.start();
    }
    focus: true
    Keys.onSpacePressed: alert();
    Timer {
        id: stopAlert
        running: false
        repeat: false
        interval: 800
        onTriggered: {
            force.enabled = false;
            mp.take(root.alertItem, true);
            centerEmitter.burst(1);
        }
    }
    Attractor {
        id: force
        system: sys
        pointX: root.width/2
        pointY: root.height/2
        strength: -10000
        enabled: false
        anchors.centerIn: parent
        width: parent.width/2
        height: parent.height/2
        groups:["photos"]
        affectedParameter: Attractor.Position
    }
    Emitter {
        id: centerEmitter
        velocity: PointDirection { x: 32; xVariation: 8;}
        emitRate: 0.5
        lifeSpan: 12000 //TODO: A -1 or something which does 'infinite'? (but need disable fade first)
        maximumEmitted: 20
        group: "photos"
        system: sys
        anchors.centerIn: parent
        enabled: false

        //TODO: Zoom in effect
    }
    Emitter {
        id: spawnFireworks
        group: "fireworks"
        system: sys
        maximumEmitted: 400
        emitRate: 400
        lifeSpan: 2800
        x: parent.width/2
        y: parent.height/2 - 64
        width: 8
        height: 8
        enabled: false
        size: 32
        endSize: 8
        velocity: AngleDirection { magnitude: 160; magnitudeVariation: 120; angleVariation: 90; angle: 270 }
        acceleration: PointDirection { y: 160 }
    }
    Item { x: -1000; y: -1000 //offscreen
        Repeater {//Load them here, add to system on completed
            model: flickrModel
            delegate: theDelegate
        }
    }
    FlickrRssModel {
        id: flickrModel
        tags: "particle,particles"
    }
    Component {
        id: theDelegate
        Image {
            required property int index
            required property string title
            required property string media
            required property string thumbnail

            id: image
            antialiasing: true;
            source: thumbnail
            cache: true
            property real depth: Math.random()
            property real darken: 0.75
            z: Math.floor(depth * 100)
            scale: (depth + 1) / 2
            sourceSize {
                width: root.width
                height: root.height
            }
            width: 132
            height: 132
            fillMode: Image.PreserveAspectFit
            Rectangle {
                // Darken based on depth
                anchors.centerIn: parent
                width: parent.paintedWidth + 1
                height: parent.paintedHeight + 1
                color: "black"
                opacity: image.darken * (1 - image.depth)
                antialiasing: true;
            }
            Text {
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottomMargin: Math.max(parent.paintedWidth, parent.paintedHeight) - Math.min(parent.width, parent.height)
                width: parent.paintedWidth - 4
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                text: image.title
                color: "black"
            }
            ItemParticle.onDetached: mp.take(image); // respawns
            onStatusChanged: if (image.status == Image.Ready) {
                loading.opacity = 0;
                mp.take(image);
            }
            function manage()
            {
                if (state == "selected") {
                    console.log("Taking " + index);
                    mp.freeze(image);
                } else {
                    console.log("Returning " +index);
                    mp.unfreeze(image);
                }
            }
            TapHandler {
                gesturePolicy: TapHandler.ReleaseWithinBounds
                onTapped: image.state = (image.state == "" ? "selected" : "")
            }
            states: State {
                name: "selected"
                ParentChange {
                    target: image
                    parent: root
                }
                PropertyChanges {
                    image {
                        source: image.media
                        x: 0
                        y: 0
                        width: root.width
                        height: root.height
                        z: 101
                        opacity: 1
                        rotation: 0
                        darken: 0
                    }
                }
            }
            transitions: Transition {
                to: "selected"
                reversible: true
                SequentialAnimation {
                    ScriptAction { script: image.manage() }
                    ParallelAnimation {
                        ParentAnimation {NumberAnimation { properties: "x,y" }}
                        PropertyAnimation { properties: "width, height, z, rotation, darken"; easing.type: Easing.InOutQuad }
                    }
                }
            }
        }
    }
}
