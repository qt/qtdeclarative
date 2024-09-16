// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window
    width: 600
    height: 400 // We want width != height to test rotated geometry

    // The derivation of this anchor setup is documented at QTBUG-123565
    background: Item {
        width: window.rotated ? window.height : window.width
        height: window.rotated ? window.width : window.height
        anchors.centerIn: parent
    }
    Overlay.overlay.anchors.fill: background
    contentItem.anchors.fill: background
    contentItem.parent.rotation: rotated ? 90 : 0

    property bool rotated: false
    property int drawerSize: 100
    property alias drawer_LR: drawer_LR
    property alias drawer_TB: drawer_TB

    Drawer {
        id: drawer_LR
        edge: Qt.LeftEdge
        width: window.drawerSize
        height: parent.height
    }
    Drawer {
        id: drawer_TB
        edge: Qt.TopEdge
        width: parent.width
        height: window.drawerSize
    }
}

