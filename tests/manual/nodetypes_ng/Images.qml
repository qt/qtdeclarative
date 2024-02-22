// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.3

Item {
    Rectangle {
        width: 100
        height: 100
        anchors.centerIn: parent
        color: "red"
        NumberAnimation on rotation { from: 0; to: 360; duration: 2000; loops: Animation.Infinite; }
    }

    Image {
        id: im
        source: "qrc:/qt.png"
        mipmap: true

        // Changing the mipmap property results in...nothing but a warning, but
        // regardless, enable the following to test.
//        Timer {
//            interval: 5000
//            onTriggered: {
//                if (im.mipmap) {
//                    console.log("disabling mipmap");
//                    im.mipmap = false;
//                } else {
//                    console.log("enabling mipmap");
//                    im.mipmap = true;
//                }
//            }
//            running: true
//            repeat: true
//        }

        SequentialAnimation on scale {
            loops: Animation.Infinite
            NumberAnimation {
                from: 1.0
                to: 4.0
                duration: 2000
            }
            NumberAnimation {
                from: 4.0
                to: 0.1
                duration: 3000
            }
            NumberAnimation {
                from: 0.1
                to: 1.0
                duration: 1000
            }
        }

        Image {
            anchors.centerIn: parent
            source: "qrc:/face-smile.png"
        }
    }

    Image {
        source: "qrc:/face-smile.png"
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        antialiasing: true // trigger smooth texture material
        NumberAnimation on rotation { from: 0; to: 360; duration: 2000; loops: Animation.Infinite; }
    }

    Item {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 10
        scale: 20
        width: 20
        Image { x: 0; source: "blacknwhite.png"; smooth: false } // solid black
        Image { x: 2; source: "blacknwhite.png"; smooth: true } // fade to white on right
        Image { x: 4; source: "blacknwhite.png"; smooth: false } // solid black
        Image { x: 6; source: "blacknwhite.png"; smooth: true } // fade to white on right
    }
}
