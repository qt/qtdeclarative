// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

GroupBox {
    id: grpBox
    title: "Add Invitees"

    property alias nextButton: nextButton
    ColumnLayout {
        anchors.fill: parent
        anchors.rightMargin: 10
        anchors.leftMargin: 10
        spacing: 10

        Text {
            id: dateTime
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
                    id: txtEdit
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
                    id: txtEmail
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
                var name = txtEdit.text
                var email = txtEmail.text
                meetingInvModel.append({
                                           "name": name,
                                           "email": email
                                       })
            }
        }
        MeetingInviteesModel {
            id: meetingInvModel
        }

        WasmListView {
            Layout.fillWidth: true
            height: 200
            listModel: meetingInvModel
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
