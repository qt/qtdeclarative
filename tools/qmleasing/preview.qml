// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Item {
    id: root
    width:  800
    height: 100

    Rectangle {
        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#aaa7a7"
            }

            GradientStop {
                position: 0.340
                color: "#a4a4a4"
            }

            GradientStop {
                position: 1
                color: "#6b6b6b"
            }
        }
        anchors.fill: parent
    }

    Button {
        id: button

        x: 19
        y: 20
        width: 133
        height: 61
        onClicked: {
            if (root.state ==="")
                root.state = "moved";
            else
                root.state = "";
        }
    }

    Rectangle {
        id: groove
        x: 163
        y: 20
        width: 622
        height: 61
        color: "#919191"
        radius: 4
        border.color: "#adadad"

        Rectangle {
            id: rectangle
            x: 9
            y: 9
            width: 46
            height: 46
            color: "#3045b7"
            radius: 4
            border.width: 2
            smooth: true
            border.color: "#9ea0bb"
            anchors.bottomMargin: 6
            anchors.topMargin: 9
            anchors.top: parent.top
            anchors.bottom: parent.bottom
        }
    }
    states: [
        State {
            name: "moved"

            PropertyChanges {
                rectangle {
                    x: 567
                    y: 9
                    anchors.bottomMargin: 6
                    anchors.topMargin: 9
                }
            }
        }
    ]

    transitions: [
        Transition {
            from: ""
            to: "moved"
            SequentialAnimation {
                PropertyAnimation {
                    easing: editor.easingCurve
                    property: "x"
                    duration: spinBox.value
                }
            }
        },
        Transition {
            from: "moved"
            to: ""
            PropertyAnimation {
                easing: editor.easingCurve
                property: "x"
                duration: spinBox.value
            }

        }
    ]
}
