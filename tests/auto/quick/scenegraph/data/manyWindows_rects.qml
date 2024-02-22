// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.2

Rectangle {
    width: 100
    height: 100

    gradient: Gradient {
        GradientStop { position: 0; color: "black" }
        GradientStop { position: 1; color: "red" }
    }

    Rectangle {
        width: 40
        height: 40
        rotation: 30
        color: "#00ff00";
        antialiasing: true
        x: parent.width / 3 - width / 2
        y: parent.height / 3 - height/ 2
    }
    Rectangle {
        width: 40
        height: 40
        rotation: 30
        color: "blue"
        x: parent.width * 2 / 3 - width / 2
        y: parent.height * 2 / 3 - height/ 2
    }
}
