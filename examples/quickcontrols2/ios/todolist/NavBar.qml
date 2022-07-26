// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ToolBar {
    id: root
    width: parent.width

    required property StackView stackView

    ToolButton {
        enabled: root.stackView.depth >= 2
        anchors.left: parent.left
        anchors.leftMargin: 5
        visible: root.stackView.depth >= 2
        anchors.verticalCenter: parent.verticalCenter
        display: AbstractButton.TextBesideIcon
        text: root.stackView.depth > 2 ? qsTr("Back") : qsTr("Home")
        font.pointSize: AppSettings.fontSize
        icon.source: "images/back.png"
        icon.height: 20
        icon.width: 20

        onClicked: root.stackView.pop()
    }

    ToolButton {
        anchors.right: parent.right
        anchors.rightMargin: 5
        icon.source: "images/settings.png"
        icon.height: 20
        icon.width: 20
        visible: {
            // Force the binding to re-evaluate so that the title check is run each time the page changes.
            root.stackView.currentItem
            !root.stackView.find((item, index) => { return item.title === "settingsPage" })
        }

        onClicked: root.stackView.push("SettingsPage.qml")
    }
}

