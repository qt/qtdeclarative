// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window
    width: 400
    height: 400

    property alias drawer: drawer

    header: Rectangle {
        color: "red"
        height: 40
    }

    Drawer {
        id: drawer
        width: window.width
        height: window.height * 0.2
        parent: window.contentItem
        edge: Qt.TopEdge

        Label {
            anchors.centerIn: parent
            text: "a drawer"
        }
    }
}
