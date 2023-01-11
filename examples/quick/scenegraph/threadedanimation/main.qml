// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [1]
import QtQuick
import Spinner

Rectangle {

    width: 320
    height: 480

    gradient: Gradient {
        GradientStop { position: 0; color: "lightsteelblue" }
        GradientStop { position: 1; color: "black" }
    }

    Rectangle {
        color: Qt.rgba(1, 1, 1, 0.7);
        radius: 10
        border.width: 1
        border.color: "white"
        anchors.fill: blockingLabel;
        anchors.margins: -10
    }

    Text {
        id: blockingLabel
        color: blocker.running ? "red" : "black"
        text: blocker.running ? qsTr("Blocked!") : qsTr("Not blocked")
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 100
    }

    Timer {
        id: blocker
        interval: 357
        running: false;
        repeat: true
        onTriggered: {
            var d = new Date();
            var x = 0;
            var wait = 50 + Math.random() * 200;
            while ((new Date().getTime() - d.getTime()) < 100) {
                x += 1;
            }
        }
    }

    Timer {
        id: blockerEnabler
        interval: 4000
        running: true
        repeat: true
        onTriggered: {
            blocker.running = !blocker.running
        }
    }

    Spinner {
        anchors.centerIn: parent
        anchors.horizontalCenterOffset: 80
        spinning: true
    }

    Image {
        anchors.centerIn: parent
        anchors.horizontalCenterOffset: -80
        source: "spinner.png"
        NumberAnimation on rotation {
            from: 0; to: 360; duration: 1000; loops: Animation.Infinite
        }
    }

    Rectangle {
        color: Qt.rgba(1, 1, 1, 0.7)
        radius: 10
        border.width: 1
        border.color: "white"
        anchors.fill: label
        anchors.margins: -10
    }

    Text {
        id: label
        color: "black"
        wrapMode: Text.WordWrap
        text: qsTr("This application shows two spinners. The one to the right is animated on the scene graph thread (when applicable) and the left one is using the normal Qt Quick animation system.")
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 20
    }
}
//! [2]
