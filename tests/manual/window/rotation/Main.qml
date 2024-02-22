// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtQuick.Controls

Window {
    id: window
    width: 640
    height: 480
    visible: true

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "white" }
            GradientStop { position: 1.0; color: "black" }
        }
        Text {
            anchors.centerIn: parent
            text: "Screen orientation: " + orientationName(Screen.orientation) + "\n" +
                  "Content orientation: " + orientationName(window.contentOrientation)
        }
    }

    Row {
        Button {
            text: "Toggle"
            onClicked: popup.visible ? popup.close() : popup.open()
            focus: true
        }
        ComboBox {
            id: orientationSelection
            model: ListModel {
                ListElement {
                    name: "Primary"
                    value: Qt.PrimaryOrientation
                }
                ListElement {
                    name: "Portrait"
                    value: Qt.PortraitOrientation
                }
                ListElement {
                    name: "Landscape"
                    value: Qt.LandscapeOrientation
                }
                ListElement {
                    name: "Inverted Portrait"
                    value: Qt.InvertedPortraitOrientation
                }
                ListElement {
                    name: "Inverted Landscape"
                    value: Qt.InvertedLandscapeOrientation
                }
            }
            textRole: "name"
            valueRole: "value"

            onActivated: updateOrientation()
        }
        Keys.onLeftPressed: (event) => {
            window.contentItem.rotation -= event.modifiers & Qt.ShiftModifier ? 10 : 1;
        }
        Keys.onRightPressed: (event) => {
            window.contentItem.rotation += event.modifiers & Qt.ShiftModifier ? 10 : 1;
        }
    }

    Popup {
        id: popup
        anchors.centerIn: parent
        width: 320
        height: 240
        modal: false
        dim: true
        Text {
            text: "Hello Popup"
            anchors.fill: parent
        }

        Overlay.modeless: Rectangle {
            opacity: 0.5
            color: "blue"
        }
    }

    Drawer {
        Text {
            anchors.centerIn: parent
            text: "Hello Left Drawer"
        }
        edge: Qt.LeftEdge
        height: parent.height
    }
    Drawer {
        Text {
            anchors.centerIn: parent
            text: "Hello Right Drawer"
        }
        edge: Qt.RightEdge
        height: parent.height
    }

    function updateOrientation() {
        window.contentOrientation = orientationSelection.currentValue;
        let angle = Screen.angleBetween(Screen.orientation, window.contentOrientation);
        console.log("Rotation between " + Screen.orientation + " and " + window.contentOrientation + " should be " + angle);
        window.contentItem.rotation = angle;
    }

    function orientationName(orientation) {
        for (let i = 0; i < orientationSelection.model.count; i++) {
            let entry = orientationSelection.model.get(i);
            if (entry.value === orientation)
                return entry.name
        }
    }

    Component.onCompleted: {
        Screen.orientationChanged.connect(updateOrientation);
        updateOrientation();
    }
}
