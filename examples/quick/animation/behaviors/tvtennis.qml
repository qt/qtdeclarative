// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: page
    width: 320; height: 480;
    color: "#1e1b18"

    // Make a ball to bounce
    Rectangle {
        id: ball

        // Add a property for the target y coordinate
        property variant direction : "right"

        x: 20; width: 20; height: 20; z: 1
        color: "#80c342"

        // Move the ball to the right and back to the left repeatedly
        SequentialAnimation on x {
            loops: Animation.Infinite
            NumberAnimation { to: page.width - 40; duration: 2000 }
            PropertyAction { target: ball; property: "direction"; value: "left" }
            NumberAnimation { to: 20; duration: 2000 }
            PropertyAction { target: ball; property: "direction"; value: "right" }
        }

        // Make y move with a velocity of 200
        Behavior on y { SpringAnimation{ velocity: 200; }
        }

        Component.onCompleted: y = page.height-10; // start the ball motion

        // Detect the ball hitting the top or bottom of the view and bounce it
        onYChanged: {
            if (y <= 0) {
                y = page.height - 20;
            } else if (y >= page.height - 20) {
                y = 0;
            }
        }
    }

    // Place bats to the left and right of the view, following the y
    // coordinates of the ball.
    Rectangle {
        id: leftBat
        color: "#328930"
        x: 2; width: 20; height: 90
        // ![0]
        y: ball.direction == 'left' ? ball.y - 45 : page.height/2 -45;
        Behavior on y { SpringAnimation{ velocity: 300 } }
        // ![0]
    }
    Rectangle {
        id: rightBat
        color: "#328930"
        x: page.width - 22; width: 20; height: 90
        y: ball.direction == 'right' ? ball.y - 45 : page.height/2 -45;
        Behavior on y { SpringAnimation{ velocity: 300 } }
    }

    // The rest, to make it look realistic, if neither ever scores...
    Rectangle { color: "#328930"; x: page.width/2-80; y: 0; width: 40; height: 60 }
    Rectangle { color: "#1e1b18"; x: page.width/2-70; y: 10; width: 20; height: 40 }
    Rectangle { color: "#328930"; x: page.width/2+40; y: 0; width: 40; height: 60 }
    Rectangle { color: "#1e1b18"; x: page.width/2+50; y: 10; width: 20; height: 40 }
    Repeater {
        model: page.height / 20
        Rectangle {
            required property int index
            color: "#328930"
            x: page.width / 2 - 5
            y: index * 20
            width: 10
            height: 10
        }
    }
}
