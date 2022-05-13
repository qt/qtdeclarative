// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

// This example shows how to create your own highlight delegate for a ListView
// that uses a SpringAnimation to provide custom movement when the
// highlight bar is moved between items.

import QtQuick
import QtQuick.Particles

Rectangle {
    id: root

    property real delegateHeight: 65
    width: 200; height: 300
    gradient: Gradient {
        GradientStop { position: 0.0; color: "#EEEEFF" }
        GradientStop { position: 1.0; color: "lightblue" }
    }

    // Define a delegate component.  A component will be
    // instantiated for each visible item in the list.
    component PetDelegate: Item {
        id: pet
        width: 200; height: root.delegateHeight
        z: 10

        required property int index
        required property string name
        required property string type
        required property int age

        Column {
            Text {color: "white"; text: pet.name; font.pixelSize: 18 }
            Text {color: "white"; text: 'Type: ' + pet.type; font.pixelSize: 14 }
            Text {color: "white"; text: 'Age: ' + pet.age; font.pixelSize: 14 }
        }
        MouseArea { anchors.fill: parent; onClicked: listView.currentIndex = pet.index; }
        // indent the item if it is the current item
        states: State {
            name: "Current"
            when: pet.ListView.isCurrentItem
            PropertyChanges { pet.x: 20 }
        }
        transitions: Transition {
            NumberAnimation { properties: "x"; duration: 200 }
        }
    }

    // Define a highlight with customized movement between items.
    component HighlightBar : Rectangle {
        z: 0
        width: 200; height: root.delegateHeight
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#99FF99" }
            GradientStop { position: 1.0; color: "#88FF88" }
        }
        y: listView.currentItem.y;
        Behavior on y { SpringAnimation { spring: 2; damping: 0.2 } }
        //! [1]
        ImageParticle {
            anchors.fill: parent
            system: particles
            source: "images/flower.png"
            color: "red"
            clip: true
            alpha: 1.0
        }
        //! [1]
    }

    ListView {
        id: listView
        width: 200; height: parent.height

        model: petsModel
        delegate: PetDelegate {}
        focus: true

        // Set the highlight delegate. Note we must also set highlightFollowsCurrentItem
        // to false so the highlight delegate can control how the highlight is moved.
        highlight: HighlightBar {}
        highlightFollowsCurrentItem: false

        ParticleSystem { id: particles }
        Emitter {
            system: particles
            anchors.fill: parent
            emitRate: 0
            lifeSpan: 10000
            size: 24
            sizeVariation: 8
            velocity: AngleDirection { angleVariation: 360; magnitude: 3 }
            maximumEmitted: 10
            startTime: 5000
            Timer { running: true; interval: 10; onTriggered: parent.emitRate = 1; }
        }

        //! [0]
        ImageParticle {
            anchors.fill: parent
            system: particles
            source: "images/flower.png"
            alpha: 0.1
            color: "white"
            rotationVariation: 180
            z: -1
        }
        //! [0]
    }

    ListModel {
        id: petsModel
        ListElement {
            name: "Polly"
            type: "Parrot"
            age: 12
            size: "Small"
        }
        ListElement {
            name: "Penny"
            type: "Turtle"
            age: 4
            size: "Small"
        }
        ListElement {
            name: "Warren"
            type: "Rabbit"
            age: 2
            size: "Small"
        }
        ListElement {
            name: "Spot"
            type: "Dog"
            age: 9
            size: "Medium"
        }
        ListElement {
            name: "Schrödinger"
            type: "Cat"
            age: 2
            size: "Medium"
        }
        ListElement {
            name: "Joey"
            type: "Kangaroo"
            age: 1
            size: "Medium"
        }
        ListElement {
            name: "Kimba"
            type: "Bunny"
            age: 65
            size: "Large"
        }
        ListElement {
            name: "Rover"
            type: "Dog"
            age: 5
            size: "Large"
        }
        ListElement {
            name: "Tiny"
            type: "Elephant"
            age: 15
            size: "Large"
        }
    }

}
