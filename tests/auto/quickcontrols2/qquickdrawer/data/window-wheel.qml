// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window
import QtQuick.Controls

Window {
    id: window
    width: 400
    height: 400

    property alias drawer: drawer
    property alias drawerSlider: drawerSlider
    property alias contentSlider: contentSlider

    Slider {
        id: contentSlider
        anchors.fill: parent
        wheelEnabled: true
    }

    Drawer {
        id: drawer
        edge: Qt.RightEdge
        width: window.width * 0.8
        height: window.height
        clip: true
        contentItem: Slider {
            id: drawerSlider
            wheelEnabled: true
        }
    }
}
