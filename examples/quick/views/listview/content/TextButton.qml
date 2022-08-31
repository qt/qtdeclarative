// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: container

    property alias text: label.text

    signal clicked

    width: label.width + 20; height: label.height + 6
    antialiasing: true
    radius: 10

    gradient: Gradient {
        GradientStop { id: gradientStop; position: 0.0; color: "#eeeeee" }
        GradientStop { position: 1.0; color: "#888888" }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: { container.clicked() }
    }

    Text {
        id: label
        anchors.centerIn: parent
    }

    states: State {
        name: "pressed"
        when: mouseArea.pressed
        PropertyChanges { gradientStop.color: "#333333" }
    }
}

