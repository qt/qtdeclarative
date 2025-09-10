// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Item {
    id: button

    signal clicked

    Rectangle {
        id: normalBackground
        radius: 4
        anchors.fill: parent
        smooth: true
        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#afafaf"
            }

            GradientStop {
                position: 0.460
                color: "#808080"
            }

            GradientStop {
                position: 1
                color: "#adadad"
            }
        }
        border.color: "#000000"
    }


    Rectangle {
        id: hoveredBackground
        x: 2
        y: -8
        radius: 4
        opacity: 0
        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#cacaca"
            }

            GradientStop {
                position: 0.460
                color: "#a2a2a2"
            }

            GradientStop {
                position: 1
                color: "#c8c8c8"
            }
        }
        smooth: true
        anchors.fill: parent
        border.color: "#000000"
    }


    Rectangle {
        id: pressedBackground
        x: -8
        y: 2
        radius: 4
        opacity: 0
        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#8b8b8b"
            }

            GradientStop {
                position: 0.470
                color: "#626161"
            }

            GradientStop {
                position: 1
                color: "#8f8e8e"
            }
        }
        smooth: true
        anchors.fill: parent
        border.color: "#000000"
    }
    states: [
        State {
            name: "hovered"

            PropertyChanges {
                normalBackground.opacity: 0
                hoveredBackground.opacity: 1
            }
        },
        State {
            name: "pressed"

            PropertyChanges {
                normalBackground.opacity: 0
                pressedBackground.opacity: 1
            }
        }
    ]

    Text {
        color: "#e8e8e8"
        text: qsTr("Play")
        anchors.fill: parent
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.bold: true
        font.pixelSize: 20
    }

    MouseArea {
        hoverEnabled: true
        anchors.fill: parent
        onEntered: button.state = "hovered"
        onExited: button.state = ""
        onClicked: {
            button.state = "pressed"
            button.clicked();
        }
    }
}
