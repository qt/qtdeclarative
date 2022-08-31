// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    id: window
    width: 320; height: 440

    // Let's draw the sky...
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#14148c" }
            GradientStop { position: 0.499; color: "#14aaff" }
            GradientStop { position: 0.5; color: "#80c342" }
            GradientStop { position: 1.0; color: "#006325" }
        }
    }

    //! [0]
    SequentialAnimation {
        SequentialAnimation {
            ParallelAnimation {
                YAnimator {
                    target: smiley;
                    from: smiley.minHeight;
                    to: smiley.maxHeight
                    easing.type: Easing.OutExpo;
                    duration: 300
                }
                ScaleAnimator {
                    target: shadow
                    from: 1
                    to: 0.5
                    easing.type: Easing.OutExpo;
                    duration: 300
                }
            }
            ParallelAnimation {
                YAnimator {
                    target: smiley;
                    from: smiley.maxHeight;
                    to: smiley.minHeight
                    easing.type: Easing.OutBounce;
                    duration: 1000
                }
                ScaleAnimator {
                    target: shadow
                    from: 0.5
                    to: 1
                    easing.type: Easing.OutBounce;
                    duration: 1000
                }
            }
        }
        PauseAnimation { duration: 500 }
        running: true
        loops: Animation.Infinite
    }
    //! [0]

    // The shadow for the smiley face
    Image {
        id: shadow;
        anchors.horizontalCenter: parent.horizontalCenter
        y: smiley.minHeight + smiley.height
        source: "images/shadow.png"
    }

    Image {
        id: smiley
        property int maxHeight: window.height / 3
        property int minHeight: 2 * window.height / 3

        anchors.horizontalCenter: parent.horizontalCenter
        y: minHeight
        source: "images/face-smile.png"

    }
}
