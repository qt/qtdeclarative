// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Image {
    id: corkPanel
    source: Qt.resolvedUrl("images/cork.jpg")
    width: ListView.view.width
    height: ListView.view.height
    fillMode: Image.PreserveAspectCrop

    required property string name
    required property var notes

    TapHandler {
        objectName: corkPanel.name
        onTapped: corkPanel.Window.activeFocusItem.focus = false
    }

    Text {
        text: corkPanel.name
        x: 15
        y: 8
        height: 40
        width: 370
        font.pixelSize: 18
        font.bold: true
        color: "white"
        style: Text.Outline
        styleColor: "black"
        wrapMode: Text.Wrap
    }

    Repeater {
        model: corkPanel.notes
        Item {
            id: fulcrum

            x: 100 + Math.random() * (corkPanel.width - 0.5 * paper.width)
            y: 50 + Math.random() * (corkPanel.height - 0.5 * paper.height)

            Item {
                id: note
                scale: 0.7

                Image {
                    id: paper
                    x: 8 + -width * 0.6 / 2
                    y: -20
                    source: "images/note-yellow.png"
                    scale: 0.6
                    transformOrigin: Item.TopLeft
                    antialiasing: true

                    DragHandler {
                        target: fulcrum
                        xAxis.minimum: 100
                        xAxis.maximum: corkPanel.width - 80
                        yAxis.minimum: 0
                        yAxis.maximum: corkPanel.height - 80
                    }
                }

                TextEdit {
                    id: text
                    x: -104
                    y: 36
                    width: 215
                    height: 24
                    font.pixelSize: 24
                    readOnly: false
                    selectByMouse: activeFocus
                    rotation: -8
                    text: noteText
                    wrapMode: Text.Wrap
                }

                rotation: -flickable.horizontalVelocity / 100
                Behavior on rotation  {
                    SpringAnimation {
                        spring: 2.0
                        damping: 0.15
                    }
                }
            }

            Image {
                x: -width / 2
                y: -height * 0.5 / 2
                source: "images/tack.png"
                scale: 0.7
                transformOrigin: Item.TopLeft
            }

            states: State {
                name: "pressed"
                when: text.activeFocus
                PropertyChanges {
                    target: note
                    rotation: 8
                    scale: 1
                }
                PropertyChanges {
                    target: fulcrum
                    z: 8
                }
            }

            transitions: Transition {
                NumberAnimation {
                    properties: "rotation,scale"
                    duration: 200
                }
            }
        }
    }
}
