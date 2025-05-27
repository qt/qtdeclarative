// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
import QtQuick

Item {

    width: 320
    height: 480

    Rectangle {
        color: "#272822"
        width: 320
        height: 480
    }

    Column {
        //![states]

        Item {
            id: container
            width: 320
            height: 120

            Rectangle {
                id: rect
                color: "red"
                width: 120
                height: 120

                TapHandler {
                    onTapped: container.state === '' ? container.state = 'other' : container.state = ''
                }
            }
            states: [
                // This adds a second state to the container where the rectangle is farther to the right

                State { name: "other"

                    PropertyChanges {
                        target: rect
                        x: 200
                    }
                }
            ]
            transitions: [
                // This adds a transition that defaults to applying to all state changes

                Transition {

                    // This applies a default NumberAnimation to any changes a state change makes to x or y properties
                    NumberAnimation { properties: "x,y" }
                }
            ]
        }
        //![states]
        //![behave]
        Item {
            width: 320
            height: 120

            Rectangle {
                color: "green"
                width: 120
                height: 120

                // This is the behavior, and it applies a NumberAnimation to any attempt to set the x property
                Behavior on x {

                    NumberAnimation {
                        //This specifies how long the animation takes
                        duration: 600
                        //This selects an easing curve to interpolate with, the default is Easing.Linear
                        easing.type: Easing.OutBounce
                    }
                }

                TapHandler {
                    onTapped: parent.x == 0 ? parent.x = 200 : parent.x = 0
                }
            }
        }
        //![behave]
        //![constant]
        Item {
            width: 320
            height: 120

            Rectangle {
                color: "blue"
                width: 120
                height: 120

                // By setting this SequentialAnimation on x, it and animations within it will automatically animate
                // the x property of this element
                SequentialAnimation on x {
                    id: xAnim
                    // Animations on properties start running by default
                    running: false
                    loops: Animation.Infinite // The animation is set to loop indefinitely
                    NumberAnimation { from: 0; to: 200; duration: 500; easing.type: Easing.InOutQuad }
                    NumberAnimation { from: 200; to: 0; duration: 500; easing.type: Easing.InOutQuad }
                    PauseAnimation { duration: 250 } // This puts a bit of time between the loop
                }

                TapHandler {
                    // The animation starts running when you click within the rectangle
                    onTapped: xAnim.running = true
                }
            }
        }
        //![constant]

        //![scripted]
        Item {
            width: 320
            height: 120

            Rectangle {
                id: rectangle
                color: "yellow"
                width: 120
                height: 120

                TapHandler {
                    // The animation starts running when you click within the rectangle
                    onTapped: anim.running = true;
                }
            }

            // This animation specifically targets the Rectangle's properties to animate
            SequentialAnimation {
                id: anim
                // Animations on their own are not running by default
                // The default number of loops is one, restart the animation to see it again

                NumberAnimation { target: rectangle; property: "x"; from: 0; to: 200; duration: 500 }

                NumberAnimation { target: rectangle; property: "x"; from: 200; to: 0; duration: 500 }
            }
        }
        //![scripted]
    }
}
//![0]
