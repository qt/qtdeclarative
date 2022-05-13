// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    width: image.implicitWidth; height: image.implicitHeight
    Image {
        id: image
        anchors.centerIn: parent
        source: "images/joystick-outer-case-pov.jpg"
        property real margin: 50

        Image {
            id: knob
            source: "images/redball.png"
            DragHandler {
                id: dragHandler
                xAxis {
                    minimum: image.margin
                    maximum: image.width - image.margin - knob.width
                }
                yAxis {
                    minimum: image.margin
                    maximum: image.height - image.margin - knob.height
                }
            }

            anchors {
                horizontalCenter: parent.horizontalCenter
                verticalCenter: parent.verticalCenter
            }
            states: [
                State {
                    when: dragHandler.active
                    AnchorChanges {
                        target: knob
                        anchors.horizontalCenter: undefined
                        anchors.verticalCenter: undefined
                    }
                }
            ]
            transitions: [
                Transition {
                    AnchorAnimation { easing.type: Easing.OutElastic }
                }
            ]
        }
    }
}
