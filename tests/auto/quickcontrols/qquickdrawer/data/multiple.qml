// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias leftDrawer: leftDrawer
    property alias leftButton: leftButton

    property alias rightDrawer: rightDrawer
    property alias rightButton: rightButton

    property alias contentButton: contentButton

    Drawer {
        id: leftDrawer
        width: 300
        height: 400
        z: 1

        contentItem: Button {
            id: leftButton
            text: "Left"
        }
    }

    Button {
        id: contentButton
        text: "Content"
        anchors.fill: parent
    }

    Drawer {
        id: rightDrawer
        width: 300
        height: 400
        edge: Qt.RightEdge

        contentItem: Button {
            id: rightButton
            text: "Right"
        }
    }
}
