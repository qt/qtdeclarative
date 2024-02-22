// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

GroupBox {
    id: grpBox
    title: "Add Invitees"
    height: parent.height - 10
    property alias nextButton: nextButton
    property alias dateAndTime: dateAndTime
    property string inviteesNameEmail
    ColumnLayout {
        id: columnLayout
        anchors.fill: parent
        anchors.rightMargin: 10
        anchors.leftMargin: 10
        spacing: 10
        Text {
            id: dateAndTime
            width: 500
            height: 50
            text: "Select Date & Time from Chrono Menu"
            font.pixelSize: 14
            Accessible.role: Accessible.StaticText
            Accessible.name: text
            Accessible.description: "This is time and date label"
        }

        RowLayout {
            id: rLayout
            spacing: 6

            Text {

                text: "Name:"
                font.pixelSize: 14
                Accessible.role: Accessible.StaticText
                Accessible.name: text
                Accessible.description: "Provide invitee's name"
            }
            Rectangle {
                width: 250
                height: 22
                color: "grey"
                border.width: 1
                border.color: "black"
                Layout.fillWidth: true
                Layout.minimumWidth: 50
                Layout.preferredWidth: 200
                Layout.maximumWidth: 300
                TextEdit {
                    id: textEdit
                    clip: true
                    anchors.fill: parent
                    anchors.leftMargin: 6
                    anchors.centerIn: parent
                    Accessible.role: Accessible.EditableText
                    Accessible.name: text
                    Accessible.description: "Write invitee's name"
                }
            }
            Text {
                text: "Email:"
                font.pixelSize: 14
                Accessible.role: Accessible.StaticText
                Accessible.name: text
                Accessible.description: "Provide invitee's Email"
            }
            Rectangle {
                width: 250
                height: 22
                color: "grey"
                border.width: 1
                border.color: "black"
                Layout.fillWidth: true
                Layout.minimumWidth: 50
                Layout.preferredWidth: 200
                Layout.maximumWidth: 300
                TextEdit {
                    id: textEmail
                    clip: true
                    width: 250
                    height: 20
                    anchors.leftMargin: 6
                    anchors.centerIn: parent
                    anchors.fill: parent
                    Accessible.role: Accessible.EditableText
                    Accessible.name: text
                    Accessible.description: "Write invitee's Email"
                }
            }
        }
        Button {
            id: addButton
            text: "Add"
            Layout.alignment: Qt.AlignRight
            anchors.rightMargin: 10
            Accessible.role: Accessible.Button
            Accessible.name: text
            Accessible.description: "Press Button to add invitee's in the list"
            onClicked: {
                var name = textEdit.text
                var email = textEmail.text
                if (inviteesNameEmail == "") {
                    inviteesNameEmail += name + "&lt;" + email + "&gt;"
                } else {
                    inviteesNameEmail += ", " + name + "&lt;" + email + "&gt;"
                }

                meetingInviteesModel.append({
                                                "name": name,
                                                "email": email
                                            })
                textEdit.text = ""
                textEmail.text = ""
            }
        }
        MeetingInviteesModel {
            id: meetingInviteesModel
        }

        WasmListView {
            Layout.fillWidth: true
            height: 200
            listModel: meetingInviteesModel
        }

        Button {
            id: nextButton
            text: "Next"
            Layout.alignment: Qt.AlignRight
            Accessible.role: Accessible.Button
            Accessible.name: text
            Accessible.description: "Press Button to go to next meeting Scheduler"
        }
    }
}
