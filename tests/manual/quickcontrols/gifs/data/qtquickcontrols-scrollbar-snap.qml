// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtQuick.Controls

Window {
    width: 100
    maximumHeight: 20
    visible: true

    property alias scrollbar: scrollbar

    ScrollBar {
        id: scrollbar
        size: 0.2
        stepSize: 0.25
        active: true
        width: parent.width
        anchors.centerIn: parent
        orientation: Qt.Horizontal
    }
}
