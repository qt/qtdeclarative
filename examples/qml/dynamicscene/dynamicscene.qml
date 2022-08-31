// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Particles
import QtQuick.Controls

Item {
    id: window

    property int activeSuns: 0
    property int centerOffset: 72

    height: 480; width: 320


    MouseArea {
        anchors.fill: parent
        onClicked: window.focus = false;
    }

    //This is the message box that pops up when there's an error
    Rectangle {
        id: dialog

        opacity: 0
        anchors.centerIn: parent
        width: dialogText.width + 6; height: dialogText.height + 6
        border.color: 'black'
        color: 'lightsteelblue'
        z: 65535 //Arbitrary number chosen to be above all the items, including the scaled perspective ones.

        function show(str){
            dialogText.text = str;
            dialogAnim.start();
        }

        Label {
            id: dialogText
            x: 3; y: 3
            font.pixelSize: 14
        }

        SequentialAnimation {
            id: dialogAnim
            NumberAnimation { target: dialog; property:"opacity"; to: 1; duration: 1000 }
            PauseAnimation { duration: 5000 }
            NumberAnimation { target: dialog; property:"opacity"; to: 0; duration: 1000 }
        }
    }

    Item {
        id: scene
        anchors { top: sky.top; bottom: ground.bottom; left: parent.left; right: parent.right}
        z: 10
    }

   // sky
    Rectangle {
        id: sky
        anchors { left: parent.left; top: toolbox.bottom; right: parent.right;  bottomMargin: -window.centerOffset; bottom: parent.verticalCenter }
        gradient: Gradient {
            GradientStop { id: gradientStopA; position: 0.0; color: "#0E1533" }
            GradientStop { id: gradientStopB; position: 1.0; color: "#437284" }
        }
    }

    // stars (when there's no sun)
    ParticleSystem {
        id: particlesystem
        anchors.fill: sky

        ImageParticle {
            id: stars
            source: "images/star.png"
            groups: ["stars"]
            opacity: .5
        }

        Emitter {
            id: starsemitter
            anchors.fill: parent
            emitRate: parent.width / 50
            lifeSpan: 5000
            group: "stars"
        }
    }

    // ground
    Rectangle {
        id: ground
        z: 2    // just above the sun so that the sun can set behind it
        anchors { left: parent.left; top: parent.verticalCenter; topMargin: window.centerOffset; right: parent.right; bottom: parent.bottom }
        gradient: Gradient {
            GradientStop { position: 0.0; color: "ForestGreen" }
            GradientStop { position: 1.0; color: "DarkGreen" }
        }
    }

    // top panel
    Rectangle {
        id: toolbox

        height: window.centerOffset * 2
        color: "white"
        anchors { right: parent.right; top: parent.top; left: parent.left}

        Column {
            anchors.centerIn: parent
            spacing: 8

            Label { text: "Drag an item into the scene." }

            Rectangle {
                width: palette.width + 10; height: palette.height + 10
                border.color: "black"

                Row {
                    id: palette
                    anchors.centerIn: parent
                    spacing: 8

                    PaletteItem {
                        anchors.verticalCenter: parent.verticalCenter
                        componentFile: "Sun.qml"
                        source: "images/sun.png"
                        image: "images/sun.png"
                    }
                    PaletteItem {
                        anchors.verticalCenter: parent.verticalCenter
                        componentFile: "GenericSceneItem.qml"
                        source: "images/moon.png"
                        image: "images/moon.png"
                    }
                    PaletteItem {
                        anchors.verticalCenter: parent.verticalCenter
                        componentFile: "PerspectiveItem.qml"
                        source: "images/tree_s.png"
                        image: "images/tree_s.png"
                    }
                    PaletteItem {
                        anchors.verticalCenter: parent.verticalCenter
                        componentFile: "PerspectiveItem.qml"
                        source: "images/rabbit_brown.png"
                        image: "images/rabbit_brown.png"
                    }
                    PaletteItem {
                        anchors.verticalCenter: parent.verticalCenter
                        componentFile: "PerspectiveItem.qml"
                        source: "images/rabbit_bw.png"
                        image: "images/rabbit_bw.png"
                    }
                }
            }

            Label { text: "Active Suns: " + window.activeSuns }
        }
    }

    //Popup toolbox down the bottom
    Rectangle {
        id: popupToolbox
        z: 1000
        width: parent.width
        height: popupColumn.height + 16
        color: "white"

        property bool poppedUp: false
        property int downY: window.height - (createButton.height + 16)
        property int upY: window.height - (popupColumn.height + 16)
        y: poppedUp ? upY : downY
        Behavior on y { NumberAnimation {}}

        Column {
            id: popupColumn
            y: 8
            spacing: 8

            Row {
                height: createButton.height
                spacing: 8
                Label { text: "Custom QML:"; anchors.verticalCenter: parent.verticalCenter }
                Button {
                    id: popupButton
                    text: popupToolbox.poppedUp ? "Hide" : "Show"
                    onClicked: popupToolbox.poppedUp = !popupToolbox.poppedUp
                }
                Button {
                    id: createButton
                    text: "Create"
                    onClicked: {
                        try {
                            Qt.createQmlObject(qmlText.text, scene, 'CustomObject');
                        } catch(err) {
                            dialog.show('Error on line ' + err.qmlErrors[0].lineNumber + '\n' + err.qmlErrors[0].message);
                        }
                    }
                }

            }

            Rectangle {
                width: 360; height: 240

                TextArea {
                    id: qmlText
                    anchors.fill: parent; anchors.margins: 5
                    readOnly: false
                    font.pixelSize: 14
                    selectByMouse: true
                    wrapMode: TextEdit.WordWrap

                    text: "import QtQuick\nImage {\n    id: smile\n    x: 360 * Math.random()\n    y: 40 * Math.random() \n    source: 'images/face-smile.png'\n    NumberAnimation on opacity { \n        to: 0; duration: 1500\n    }\n    Component.onCompleted: smile.destroy(1500);\n}"
                }
            }
        }
    }

    //Day state, for when a sun is added to the scene
    states: State {
        name: "Day"
        when: window.activeSuns > 0

        PropertyChanges {
            gradientStopA.color: "DeepSkyBlue"
            gradientStopB.color: "SkyBlue"
            stars.opacity: 0
        }
    }

    //! [top-level transitions]
    transitions: Transition {
        PropertyAnimation { duration: 3000 }
        ColorAnimation { duration: 3000 }
    }
    //! [top-level transitions]
}
