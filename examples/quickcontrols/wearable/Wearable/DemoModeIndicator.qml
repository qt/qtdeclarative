// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls as QQC2
import WearableStyle
import QtQuick.Effects

Item {
    id: indicator
    width: row.implicitWidth + margins * 2
    height: row.implicitHeight + margins * 2

    opacity: 0.8

    readonly property int margins: 12

    Behavior on y {
        NumberAnimation {}
    }

    Rectangle {
        id: demoModeIndicatorBg
        anchors.fill: parent
        radius: 20
        color: UIStyle.colorRed
    }

    MultiEffect {
        source: demoModeIndicatorBg
        anchors.fill: parent
        shadowEnabled: true
        shadowBlur: 0.3
        shadowHorizontalOffset: 2
        shadowVerticalOffset: 2
        opacity: 0.5
    }
    Row {
        id: row
        spacing: 8
        anchors.fill: parent
        anchors.leftMargin: parent.margins
        anchors.rightMargin: parent.margins

        Image {
            source: UIStyle.iconPath("demomode")
            anchors.verticalCenter: parent.verticalCenter
        }
        QQC2.Label {
            id: instructionLabel
            text: qsTr("Tap screen to use")
            font: UIStyle.h3
            color: UIStyle.textColor
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
