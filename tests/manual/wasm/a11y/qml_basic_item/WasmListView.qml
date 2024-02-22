// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: listRect
    width: parent.width
    height: parent.height / 2
    color: "bisque"
    property alias listModel: wasmList.model

    ListView {
        id: wasmList
        anchors.fill: listRect
        clip: true
        Accessible.role: Accessible.List
        Accessible.name: "ListView"
        Accessible.description: "List view to add the names and emails of the invitees"
        headerPositioning: ListView.OverlayHeader
        focus: true
        header: Component {
            Row {
                z: 2
                spacing: 2
                Rectangle {
                    color: "#808080"
                    Text {
                        text: "Name"
                        width: listRect.width / 2
                        font.bold: true
                        horizontalAlignment: Text.AlignLeft
                        Layout.fillWidth: true
                        color: "black"
                        Accessible.role: Accessible.StaticText
                        Accessible.name: text
                        Accessible.description: "Invitee's name"
                    }
                    width: childrenRect.width
                    height: childrenRect.height
                }
                Rectangle {
                    color: "#808080"
                    Text {
                        text: "Email"
                        width: listRect.width / 2
                        color: 'black'
                        font.bold: true
                        horizontalAlignment: Text.AlignLeft
                        Layout.fillWidth: true
                        Accessible.role: Accessible.StaticText
                        Accessible.name: text
                        Accessible.description: "Invitee's email address"
                    }
                    width: childrenRect.width
                    height: childrenRect.height
                }
            }
        }
        delegate: Rectangle {
            width: wasmList.width
            height: 50
            color: index % 2 === 0 ? "#F5F5F5" : "#FFFFFF"

            Text {
                id: inviteeName
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                leftPadding: 10
                width: listRect.width / 2
                text: model.name
                font.bold: true
                Accessible.role: Accessible.ListItem
                Accessible.name: text
                Accessible.description: text
                color: "black"
            }

            Text {
                id: inviteeEmail
                width: listRect.width / 2
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                rightPadding: 10
                text: model.email
                font.bold: true
                color: "black"
                Accessible.role: Accessible.ListItem
                Accessible.name: text
                Accessible.description: text
            }
        }
    }
}
