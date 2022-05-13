// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Window

ApplicationWindow {
    id: window
    width: 1280
    height: 720
    visible: true
    title: "Qt Quick Controls - Imagine Style Example: Music Player"

    Component.onCompleted: {
        x = Screen.width / 2 - width / 2
        y = Screen.height / 2 - height / 2
    }

    Shortcut {
        sequence: "Ctrl+Q"
        onActivated: Qt.quit()
    }

    header: ToolBar {
        RowLayout {
            id: headerRowLayout
            anchors.fill: parent
            spacing: 0

            ToolButton {
                icon.name: "grid"
            }
            ToolButton {
                icon.name: "settings"
            }
            ToolButton {
                icon.name: "filter"
            }
            ToolButton {
                icon.name: "message"
            }
            ToolButton {
                icon.name: "music"
            }
            ToolButton {
                icon.name: "cloud"
            }
            ToolButton {
                icon.name: "bluetooth"
            }
            ToolButton {
                icon.name: "cart"
            }

            Item {
                Layout.fillWidth: true
            }

            ToolButton {
                icon.name: "power"
                onClicked: Qt.quit()
            }
        }
    }

    Label {
        text: "Qtify"
        font.pixelSize: Qt.application.font.pixelSize * 1.3
        anchors.centerIn: header
        z: header.z + 1
    }

    RowLayout {
        spacing: 115
        anchors.fill: parent
        anchors.margins: 70

        ColumnLayout {
            spacing: 0
            Layout.preferredWidth: 230

            RowLayout {
                Layout.maximumHeight: 170

                ColumnLayout {
                    Label {
                        text: "12 dB"
                        Layout.fillHeight: true
                    }
                    Label {
                        text: "6 dB"
                        Layout.fillHeight: true
                    }
                    Label {
                        text: "0 dB"
                        Layout.fillHeight: true
                    }
                    Label {
                        text: "-6 dB"
                        Layout.fillHeight: true
                    }
                    Label {
                        text: "-12 dB"
                        Layout.fillHeight: true
                    }
                }

                Repeater {
                    model: 7

                    Slider {
                        value: Math.random()
                        orientation: Qt.Vertical

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }
                }
            }

            RowLayout {
                spacing: 10
                Layout.topMargin: 23

                ComboBox {
                    currentIndex: 1
                    model: ["Blues", "Classical", "Jazz", "Metal"]
                    Layout.fillWidth: true
                }

                Button {
                    icon.name: "folder"
                }

                Button {
                    icon.name: "save"
                    enabled: false
                }
            }

            Dial {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 50
            }

            Label {
                text: "Volume"

                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 12
            }
        }

        ColumnLayout {
            spacing: 26
            Layout.preferredWidth: 230

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                Image {
                    anchors.fill: parent
                    fillMode: Image.PreserveAspectCrop
                    source: "images/album-cover.jpg"
                }
            }

            Item {
                id: songLabelContainer
                clip: true

                Layout.fillWidth: true
                Layout.preferredHeight: songNameLabel.implicitHeight

                SequentialAnimation {
                    running: true
                    loops: Animation.Infinite

                    PauseAnimation {
                        duration: 2000
                    }
                    ParallelAnimation {
                        XAnimator {
                            target: songNameLabel
                            from: 0
                            to: songLabelContainer.width - songNameLabel.implicitWidth
                            duration: 5000
                        }
                        OpacityAnimator {
                            target: leftGradient
                            from: 0
                            to: 1
                        }
                    }
                    OpacityAnimator {
                        target: rightGradient
                        from: 1
                        to: 0
                    }
                    PauseAnimation {
                        duration: 1000
                    }
                    OpacityAnimator {
                        target: rightGradient
                        from: 0
                        to: 1
                    }
                    ParallelAnimation {
                        XAnimator {
                            target: songNameLabel
                            from: songLabelContainer.width - songNameLabel.implicitWidth
                            to: 0
                            duration: 5000
                        }
                        OpacityAnimator {
                            target: leftGradient
                            from: 0
                            to: 1
                        }
                    }
                    OpacityAnimator {
                        target: leftGradient
                        from: 1
                        to: 0
                    }
                }

                Rectangle {
                    id: leftGradient
                    gradient: Gradient {
                        GradientStop {
                            position: 0
                            color: "#dfe4ea"
                        }
                        GradientStop {
                            position: 1
                            color: "#00dfe4ea"
                        }
                    }

                    width: height
                    height: parent.height
                    anchors.left: parent.left
                    z: 1
                    rotation: -90
                    opacity: 0
                }

                Label {
                    id: songNameLabel
                    text: "Edvard Grieg - In the Hall of the Mountain King"
                    font.pixelSize: Qt.application.font.pixelSize * 1.4
                }

                Rectangle {
                    id: rightGradient
                    gradient: Gradient {
                        GradientStop {
                            position: 0
                            color: "#00dfe4ea"
                        }
                        GradientStop {
                            position: 1
                            color: "#dfe4ea"
                        }
                    }

                    width: height
                    height: parent.height
                    anchors.right: parent.right
                    rotation: -90
                }
            }

            RowLayout {
                spacing: 8
                Layout.alignment: Qt.AlignHCenter

                RoundButton {
                    icon.name: "favorite"
                    icon.width: 32
                    icon.height: 32
                }
                RoundButton {
                    icon.name: "stop"
                    icon.width: 32
                    icon.height: 32
                }
                RoundButton {
                    icon.name: "previous"
                    icon.width: 32
                    icon.height: 32
                }
                RoundButton {
                    icon.name: "pause"
                    icon.width: 32
                    icon.height: 32
                }
                RoundButton {
                    icon.name: "next"
                    icon.width: 32
                    icon.height: 32
                }
                RoundButton {
                    icon.name: "repeat"
                    icon.width: 32
                    icon.height: 32
                }
                RoundButton {
                    icon.name: "shuffle"
                    icon.width: 32
                    icon.height: 32
                }
            }

            Slider {
                id: seekSlider
                value: 113
                to: 261

                Layout.fillWidth: true

                ToolTip {
                    parent: seekSlider.handle
                    visible: seekSlider.pressed
                    text: pad(Math.floor(value / 60)) + ":" + pad(Math.floor(value % 60))
                    y: parent.height

                    readonly property int value: seekSlider.valueAt(seekSlider.position)

                    function pad(number) {
                        if (number <= 9)
                            return "0" + number;
                        return number;
                    }
                }
            }
        }

        ColumnLayout {
            spacing: 16
            Layout.preferredWidth: 230

            ButtonGroup {
                buttons: libraryRowLayout.children
            }

            RowLayout {
                id: libraryRowLayout
                Layout.alignment: Qt.AlignHCenter

                Button {
                    text: "Files"
                    checked: true
                }
                Button {
                    text: "Playlists"
                    checkable: true
                }
                Button {
                    text: "Favourites"
                    checkable: true
                }
            }

            RowLayout {
                TextField {
                    Layout.fillWidth: true
                }
                Button {
                    icon.name: "folder"
                }
            }

            Frame {
                id: filesFrame
                leftPadding: 1
                rightPadding: 1

                Layout.fillWidth: true
                Layout.fillHeight: true

                ListView {
                    id: filesListView
                    clip: true
                    anchors.fill: parent
                    model: ListModel {
                        Component.onCompleted: {
                            for (var i = 0; i < 100; ++i) {
                                append({
                                   author: "Author",
                                   album: "Album",
                                   track: "Track 0" + (i % 9 + 1),
                                });
                            }
                        }
                    }
                    delegate: ItemDelegate {
                        text: model.author + " - " + model.album + " - " + model.track
                        width: filesListView.width
                    }

                    ScrollBar.vertical: ScrollBar {
                        parent: filesFrame
                        policy: ScrollBar.AlwaysOn
                        anchors.top: parent.top
                        anchors.topMargin: filesFrame.topPadding
                        anchors.right: parent.right
                        anchors.rightMargin: 1
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: filesFrame.bottomPadding
                    }
                }
            }
        }
    }
}
