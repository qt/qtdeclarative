// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    width: 64
    height: 64
    property alias value: list.currentIndex
    property alias label: caption.text

    Text {
        id: caption
        text: "Spinner"
        anchors.horizontalCenter: parent.horizontalCenter
    }

    Rectangle {
        anchors.top: caption.bottom
        anchors.topMargin: 4
        anchors.horizontalCenter: parent.horizontalCenter
        height: 48
        width: 32
        color: "black"
        ListView {
            id: list
            anchors.fill: parent
            highlightRangeMode: ListView.StrictlyEnforceRange
            preferredHighlightBegin: height/3
            preferredHighlightEnd: height/3
            clip: true
            model: 64
            delegate: Text {
                required property int index
                font.pixelSize: 18;
                color: "white";
                text: index;
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#FF000000" }
                GradientStop { position: 0.2; color: "#00000000" }
                GradientStop { position: 0.8; color: "#00000000" }
                GradientStop { position: 1.0; color: "#FF000000" }
            }
        }
    }
}
