// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// This example was created for the blog post about responsive layouts:
// https://www.qt.io/blog/responsive-layouts-in-qt

import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls

Window {
    id: window

    width: 700
    height: 800

    minimumHeight: 500
    minimumWidth: 200

    title: "Window: (" + width + "x" + height + ")"
    visible: true

    Component {
        id: delegate
        Rectangle {
            width: listView.width
            height: 70
            Rectangle {
                id: circ
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 10
                height: 50
                width: 50
                radius: 25
                color: "#c8c8c8"
            }
            Rectangle {
                anchors.left: circ.right
                anchors.right: parent.right
                anchors.bottom: circ.verticalCenter
                anchors.leftMargin: 10
                anchors.rightMargin: 90
                anchors.bottomMargin: 5
                height: 11
                color: "#c8c8c8"
            }
            Rectangle {
                anchors.left: circ.right
                anchors.right: parent.right
                anchors.top: circ.verticalCenter
                anchors.leftMargin: 10
                anchors.rightMargin: 90
                anchors.topMargin: 5
                height: 11
                color: "#c8c8c8"
            }
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: "#eaeaea"
            }
            Rectangle {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 1
                color: "#eaeaea"
            }
        }
    }

    ListView {
        id: listView
        z: 2
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumWidth: 300
        Layout.maximumWidth: 550

        model: 20
        delegate: delegate

        TapHandler {
            onTapped: swipeView.currentIndex = 1
        }
    }

    GridLayout {
        z: 2
        id: detailView

        Layout.minimumWidth: 250
        Layout.fillHeight: false
        Layout.fillWidth: true
        Layout.leftMargin: 15
        Layout.rightMargin: 15
        Layout.alignment: Qt.AlignTop

        columns: Math.max(width,100)/10
        columnSpacing: 0
        rowSpacing: 10
        Rectangle {
            Layout.columnSpan: detailView.columns
            Layout.fillWidth: true
            height: 300
            color: "#c8c8c8"
        }
        Rectangle {
            Layout.columnSpan: detailView.columns
            Layout.fillWidth: true
            Layout.fillHeight: false
            height: 10
        }
        Repeater {
            model: 1000
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: false
                height: 10
                color: "#c8c8c8"
            }
        }
    }

    LayoutChooser {
        id: layoutChooser
        width: parent.width
        height: parent.height

        layoutChoices: [
            smallLayout,
            largeLayout
        ]

        criteria: [
            window.width < listView.Layout.minimumWidth + detailView.Layout.minimumWidth + 20,
            true
        ]

        property Item smallLayout: ColumnLayout {
            parent: layoutChooser
            height: parent.height
            width: parent.width

            SwipeView {
                id: swipeView

                Layout.fillHeight: true
                Layout.fillWidth: true
                Item {
                    ColumnLayout {
                        height: parent.height
                        width: parent.width

                        spacing: 0
                        Rectangle {
                            height: 50
                            Layout.fillWidth: true
                        }
                        Rectangle {
                            height: 1
                            color: "#eaeaea"
                            Layout.fillWidth: true
                        }
                        LayoutItemProxy { target: listView }
                    }
                }
                Item {
                    ColumnLayout {
                        height: parent.height
                        width: parent.width
                        Rectangle {
                            height: 50
                            Layout.fillWidth: true

                            Text {
                                id: im
                                FontLoader {
                                    id: materialFont
                                    source: "https://github.com/google/material-design-icons/blob/master/font/MaterialIcons-Regular.ttf?raw=true"
                                }
                                font.family: materialFont.font.family
                                font.weight: materialFont.font.weight
                                font.pixelSize: 32
                                anchors.left: parent.left
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.leftMargin: 10
                                text: String.fromCodePoint(0xe5c4)
                                color: "#010101"
                                TapHandler {
                                   onTapped: { swipeView.currentIndex = 0; }
                                }
                            }
                        }

                        Rectangle {
                            height: 1
                            color: "#eaeaea"
                            Layout.fillWidth: true
                        }

                        LayoutItemProxy { target: detailView }
                    }
                }
            }
        }

        property Item largeLayout: ColumnLayout {
            parent: layoutChooser
            height: parent.height
            width: parent.width

            spacing: 0
            Rectangle {
                height: 50
                Layout.fillWidth: true
            }
            Rectangle {
                height: 1
                color: "#eaeaea"
                Layout.fillWidth: true
            }

            RowLayout {
                spacing: 0
                LayoutItemProxy { target: listView }
                LayoutItemProxy { target: detailView }
            }
        }
    }

}
