// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtCore
import QtQuick
import QtQuick.Controls as QQC2

import WearableSettings
import WearableStyle

QQC2.ApplicationWindow {
    id: window
    visible: true
    width: 320
    height: 320
    title: qsTr("Wearable")

    background: Image {
        source: UIStyle.themeImagePath("background")
    }

    header: NaviButton {
        id: homeButton
        edge: Qt.TopEdge
        enabled: stackView.depth > 1
        imageSource: UIStyle.imagePath("home")

        onClicked: stackView.pop(null)
    }

    footer: NaviButton {
        id: backButton
        edge: Qt.BottomEdge
        enabled: stackView.depth > 1
        imageSource: UIStyle.imagePath("back")

        onClicked: stackView.pop()
    }

    QQC2.StackView {
        id: stackView

        focus: true
        anchors.fill: parent

        initialItem: LauncherPage {
            onLaunched: (page) => stackView.push(page)
        }
    }

    DemoMode {
        stackView: stackView
    }

    DemoModeIndicator {
        id: demoModeIndicator
        y: WearableSettings.demoMode ? -height : -height * 2
        anchors.horizontalCenter: parent.horizontalCenter
        height: header.height
        z: window.header.z + 1
    }

    MouseArea {
        enabled: WearableSettings.demoMode
        anchors.fill: parent
        onClicked: {
            // Stop demo mode and return to the launcher page.
            WearableSettings.demoMode = false
            stackView.pop(null)
        }
    }
}
