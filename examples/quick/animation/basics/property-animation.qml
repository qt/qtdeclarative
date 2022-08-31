// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQml
import QtQuick

Item {
    id: window
    width: 320; height: 480

    // Let's draw the sky...
    Rectangle {
        anchors { left: parent.left; top: parent.top; right: parent.right; bottom: parent.verticalCenter }
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#14148c" }
            GradientStop { position: 1.0; color: "#14aaff" }
        }
    }

    // ...and the ground.
    Rectangle {
        anchors { left: parent.left; top: parent.verticalCenter; right: parent.right; bottom: parent.bottom }
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#80c342" }
            GradientStop { position: 1.0; color: "#006325" }
        }
    }

    // The shadow for the smiley face
    Image {
        anchors.horizontalCenter: parent.horizontalCenter
        y: smiley.minHeight + 58
        source: "images/shadow.png"

        // The scale property depends on the y position of the smiley face.
        scale: smiley.y * 0.5 / (smiley.minHeight - smiley.maxHeight)
    }

    Image {
        id: smiley
        property int maxHeight: window.height / 3
        property int minHeight: 2 * window.height / 3

        anchors.horizontalCenter: parent.horizontalCenter
        y: minHeight
        source: "images/face-smile.png"

        //! [0]
        // Animate the y property. Setting loops to Animation.Infinite makes the
        // animation repeat indefinitely, otherwise it would only run once.
        SequentialAnimation on y {
            loops: Animation.Infinite

            // Move from minHeight to maxHeight in 300ms, using the OutExpo easing function
            NumberAnimation {
                from: smiley.minHeight; to: smiley.maxHeight
                easing.type: Easing.OutExpo; duration: 300
            }

            // Then move back to minHeight in 1 second, using the OutBounce easing function
            NumberAnimation {
                from: smiley.maxHeight; to: smiley.minHeight
                easing.type: Easing.OutBounce; duration: 1000
            }

            // Then pause for 500ms
            PauseAnimation { duration: 500 }
        }
        //! [0]
    }
}
