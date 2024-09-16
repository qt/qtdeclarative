// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Rectangle {
    id: rectangle
    width: 500; height: 500
    color: "lightgray"

    Text {
        id: text;
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -50
        font.pointSize: 40
        text: greeter.greeting
    }

    Image {
        anchors.top: text.bottom
        anchors.topMargin: 5
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: -8
        source: "https://www.swift.org/assets/images/swift.svg"
        fillMode: Image.PreserveAspectFit
        sourceSize.width: 150

        MouseArea {
            anchors.fill: parent
            onClicked: greeter.updateGreeting();
        }
    }
}
