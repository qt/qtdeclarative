// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import chattutorial

Page {
    id: root

    header: ChatToolBar {
        Label {
            text: qsTr("Contacts")
            font.pixelSize: 20
            anchors.centerIn: parent
        }
    }

    ListView {
        id: listView
        anchors.fill: parent
        topMargin: 48
        leftMargin: 48
        bottomMargin: 48
        rightMargin: 48
        spacing: 20
        model: SqlContactModel {}
        delegate: ItemDelegate {
            id: contactDelegate
            text: model.display
            width: listView.width - listView.leftMargin - listView.rightMargin
            height: avatar.implicitHeight
            leftPadding: avatar.implicitWidth + 32

            // Use "model" rather than the specific "display" role, because it
            // would conflict with the display property of ItemDelegate.
            required property var model

            onClicked: root.StackView.view.push("ConversationPage.qml", { inConversationWith: model.display })

            Image {
                id: avatar
                source: "images/" + contactDelegate.model.display.replace(" ", "_") + ".png"
            }
        }
    }
}

