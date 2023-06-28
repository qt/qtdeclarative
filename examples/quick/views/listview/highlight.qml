// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

// This example shows how to create your own highlight delegate for a ListView
// that uses a SpringAnimation to provide custom movement when the
// highlight bar is moved between items.

import QtQuick
import "content"

Rectangle {
    width: 200
    height: 300

    // Define a delegate component. The component will be
    // instantiated for each visible item in the list.
    component PetDelegate: Item {
        id: pet
        width: 200
        height: 55

        required property int index
        required property string name
        required property string type
        required property int age

        Column {
            SmallText {
                text: 'Name: ' + pet.name
            }
            SmallText {
                text: 'Type: ' + pet.type
            }
            SmallText {
                text: 'Age: ' + pet.age
            }
        }
        // indent the item if it is the current item
        states: State {
            name: "Current"
            when: pet.ListView.isCurrentItem
            PropertyChanges { pet.x: 20 }
        }
        transitions: Transition {
            NumberAnimation {
                properties: "x"
                duration: 200
            }
        }
        MouseArea {
            anchors.fill: parent
            onClicked: pet.ListView.view.currentIndex = pet.index
        }
    }

//! [0]
    // Define a highlight with customized movement between items.
    component HighlightBar : Rectangle {
        width: 200
        height: 50
        color: "#FFFF88"
        y: ListView.view.currentItem.y
        Behavior on y {
            SpringAnimation {
                spring: 2
                damping: 0.1
            }
        }
    }

    ListView {
        id: listView
        width: 200
        height: parent.height
        x: 30

        model: PetsModel { }
        delegate: PetDelegate { }
        focus: true

        // Set the highlight delegate. Note we must also set highlightFollowsCurrentItem
        // to false so the highlight delegate can control how the highlight is moved.
        highlight: HighlightBar { }
        highlightFollowsCurrentItem: false
    }
//! [0]
}
