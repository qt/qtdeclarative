// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window
    width: 600
    height: 400

    property real scaleFactor: 1

    background: Rectangle {
        objectName: "background"
        color: "green"
        width: window.width * window.scaleFactor
        height: window.height * window.scaleFactor
        anchors.centerIn: parent
    }
}
