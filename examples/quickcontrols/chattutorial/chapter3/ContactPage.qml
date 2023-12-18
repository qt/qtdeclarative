// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

Page {
    id: root

    header: ToolBar {
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
        model: ["Albert Einstein", "Ernest Hemingway", "Hans Gude"]
        delegate: ItemDelegate {
            id: contactDelegate
            text: modelData
            width: listView.width - listView.leftMargin - listView.rightMargin
            height: avatar.implicitHeight
            leftPadding: avatar.implicitWidth + 32

            required property string modelData

            onClicked: root.StackView.view.push("ConversationPage.qml", { inConversationWith: modelData })

            Image {
                id: avatar
                source: "images/" + contactDelegate.modelData.replace(" ", "_") + ".png"
            }
        }
    }
}

