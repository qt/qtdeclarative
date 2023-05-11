// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls as QQC2
import WearableStyle

Item {
    id: root
    width: row.implicitWidth + margins * 2

    readonly property int topMargin: 24
    readonly property int margins: 12

    Behavior on y {
        NumberAnimation {}
    }

    Rectangle {
        id: demoModeIndicatorBg
        anchors.fill: parent
        anchors.topMargin: -topMargin
        radius: 20
        color: UIStyle.colorRed
    }

    Row {
        id: row
        spacing: 8
        anchors.fill: parent
        anchors.leftMargin: margins
        anchors.rightMargin: margins

        Image {
            source: UIStyle.imagePath("settings-demo-mode-white")
            width: height
            height: instructionLabel.height * 2
            anchors.verticalCenter: parent.verticalCenter
        }
        QQC2.Label {
            id: instructionLabel
            text: "Tap screen to use"
            color: UIStyle.colorQtGray10
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
