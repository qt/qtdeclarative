// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.1
import QtQuick.Window
import QtQuick.Controls

Window {
    width: 800
    height: 700
    visible: true

    Flow {
        anchors.fill: parent
        spacing: 20

        Column {
            Label {
                text: "Nested: MouseArea, MouseArea"
            }
            Rectangle {
                width: 200
                height: 200
                border.width: 1
                color: m3.containsMouse ? "yellow" : "transparent"
                MouseArea {
                    id: m3
                    anchors.fill: parent
                    hoverEnabled: true

                    MouseArea {
                        id: m2
                        width: 100
                        height: 100
                        hoverEnabled: true
                        Rectangle {
                            anchors.fill: parent
                            color: m2.containsMouse ? "green" : "transparent"
                            opacity: 0.5
                            border.width: 1
                        }
                    }
                }
            }
        }

        Column {
            Label {
                text: "Nested: MouseArea, Rectangle, MouseArea"
            }
            Rectangle {
                width: 200
                height: 200
                border.width: 1
                color: m4.containsMouse ? "yellow" : "transparent"
                MouseArea {
                    id: m4
                    anchors.fill: parent
                    hoverEnabled: true

                    Rectangle {
                        width: 100
                        height: 100
                        color: m1.containsMouse ? "green" : "transparent"
                        opacity: 0.5
                        border.width: 1
                        MouseArea {
                            id: m1
                            anchors.fill: parent
                            hoverEnabled: true
                        }
                    }
                }
            }
        }

        Column {
            Label {
                text: "Siblings: MouseArea, MouseArea"
            }
            Item {
                width: 200
                height: 200
                MouseArea {
                    id: ma1
                    width: 150
                    height: 150
                    hoverEnabled: true
                    Rectangle {
                        anchors.fill: parent
                        color: ma1.containsMouse ? "red" : "white"
                        opacity: 0.5
                        border.width: 1
                    }
                }
                MouseArea {
                    id: ma2
                    x: 50
                    y: 50
                    width: 150
                    height: 150
                    hoverEnabled: true
                    Rectangle {
                        anchors.fill: parent
                        color: ma2.containsMouse ? "blue" : "white"
                        opacity: 0.5
                        border.width: 1
                    }
                }
            }
        }

        Column {
            Label {
                text: "Nested: Button, Button"
            }

            Button {
                width: 200
                height: 200
                hoverEnabled: true
                text: hovered ? "hovered" : ""

                Button {
                    width: 100
                    height: 100
                    text: hovered ? "hovered" : ""
                }
            }
        }

        Column {
            Label {
                text: "Siblings: Button, Button"
            }

            Item {
                width: 200
                height: 200
                Button {
                    width: 150
                    height: 150
                    hoverEnabled: true
                    text: hovered ? "hovered" : ""
                }

                Button {
                    x: 50
                    y: 50
                    width: 150
                    height: 150
                    text: hovered ? "hovered" : ""
                    opacity: 0.8
                }
            }
        }

        Column {
            Label {
                text: "Siblings: Button, MouseArea"
            }
            Item {
                width: 200
                height: 200
                Button {
                    width: 150
                    height: 150
                    text: hovered ? "hovered" : ""
                }

                MouseArea {
                    id: m8
                    x: 50
                    y: 50
                    width: 150
                    height: 150
                    hoverEnabled: true
                    Rectangle {
                        anchors.fill: parent
                        color: m8.containsMouse ? "blue" : "transparent"
                        opacity: 0.5
                        border.width: 1
                    }
                }
            }
        }

        Column {
            Label {
                text: "Siblings: MouseArea, Button"
            }
            Item {
                width: 200
                height: 200

                MouseArea {
                    id: ma11
                    width: 150
                    height: 150
                    hoverEnabled: true
                    Rectangle {
                        anchors.fill: parent
                        color: ma11.containsMouse ? "blue" : "transparent"
                        opacity: 0.5
                        border.width: 1
                    }
                }

                Button {
                    x: 50
                    y: 50
                    width: 150
                    height: 150
                    text: hovered ? "hovered" : ""
                    opacity: 0.8
                }
            }
        }

        Column {
            Label {
                text: "Nested: Button, Rectangle, MouseArea, Rectangle, MouseArea"
            }
            Button {
                width: 200
                height: 200
                text: hovered ? "hovered" : ""
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 20
                    border.width: 1
                    color: m5.containsMouse ? "yellow" : "transparent"
                    opacity: 0.5
                    MouseArea {
                        id: m5
                        anchors.fill: parent
                        hoverEnabled: true

                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: 20
                            color: m6.containsMouse ? "green" : "transparent"
                            opacity: 0.5
                            border.width: 1
                            MouseArea {
                                id: m6
                                anchors.fill: parent
                                hoverEnabled: true
                            }
                        }
                    }
                }
            }
        }
    }
}
