// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

SplitView {
    anchors.fill: parent
    orientation: Qt.Vertical

    Rectangle {
        objectName: "salmon"
        color: objectName
        implicitWidth: 25
        implicitHeight: 25

        SplitView.fillHeight: true
    }
    Rectangle {
        objectName: "navajowhite"
        color: objectName
        implicitWidth: 200
        implicitHeight: 200
    }
    Rectangle {
        objectName: "steelblue"
        color: objectName
        implicitWidth: 200
        implicitHeight: 200
    }
}
