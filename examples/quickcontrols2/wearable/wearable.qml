// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtCore
import QtQuick
import QtQuick.Controls as QQC2
import "qml"
import "qml/Style"

QQC2.ApplicationWindow {
    id: window
    visible: true
    width: 320
    height: 320
    title: qsTr("Wearable")

    Settings {
        id: settings
        property bool wireless
        property bool bluetooth
        property int brightness
        property bool darkTheme
        property bool demoMode
    }

    Binding {
        target: UIStyle
        property: "darkTheme"
        value: settings.darkTheme
    }

    // We need the settings object both here and in SettingsPage,
    // so for convenience, we declare it as a property of the root object so that
    // it will be available to all of the QML files that we load.
    property alias settings: settings

    background: Image {
        source: "images/background-" + (settings.darkTheme ? "dark" : "light") + ".png"
    }

    header: NaviButton {
        id: homeButton

        edge: Qt.TopEdge
        enabled: stackView.depth > 1
        imageSource: "images/home.png"

        onClicked: stackView.pop(null)
    }

    footer: NaviButton {
        id: backButton

        edge: Qt.BottomEdge
        enabled: stackView.depth > 1
        imageSource: "images/back.png"

        onClicked: stackView.pop()
    }

    QQC2.StackView {
        id: stackView

        focus: true
        anchors.fill: parent

        initialItem: LauncherPage {
            onLaunched: stackView.push(page)
        }
    }

    DemoMode {
        stackView: stackView
    }

    DemoModeIndicator {
        id: demoModeIndicator
        y: settings.demoMode ? -height : -height * 2
        anchors.horizontalCenter: parent.horizontalCenter
        height: header.height
        z: window.header.z + 1
    }

    MouseArea {
        enabled: settings.demoMode
        anchors.fill: parent
        onClicked: {
            // Stop demo mode and return to the launcher page.
            settings.demoMode = false
            stackView.pop(null)
        }
    }
}
