// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GroupBox {
    title: "Summary"
    property string meetingOccurrence: "Once"
    property string onlineOfflineStatus: "offline"
    property int roomNumber: 0
    property int calendarWeek: 1
    property string meetingDescription: "No Description"
    property string inviteesNameEmail: ""
    property bool addedReadRequest: false

    ColumnLayout {
        id: columnLayout
        width: 500
        height: 200
        spacing: 2
        Label {
            Layout.preferredHeight: 30
            text: "Meeting Details"
            font.bold: true
            font.pixelSize: 24
            Accessible.role: Accessible.StaticText
            Accessible.name: text
            Accessible.description: "Title"
        }

        Connections {
            target: wasmToolbar
            function onCancelThisMeeting() {
                textSummary.text = ""
                addedReadRequest = false
            }
            function onRequestReadReceipt() {
                if (addedReadRequest == false) {
                    var temp = textSummary.text
                    textSummary.text = "<b>Read receipt requested</b><br>" + temp
                    addedReadRequest = true
                }
            }
        }
        Text {
            id: textSummary
            Layout.alignment: Qt.AlignLeft
            Layout.leftMargin: 10
            Layout.preferredWidth: 300
            font.pixelSize: 16
            textFormat: Text.StyledText
            text: qsTr((" Occurrence:<b> %1 </b> <br>
                          Meeting to be held: <b>%2</b> <br>
                          Invitees:<b> %3 </b> <br>
                          Meeting Room: <b> %4 </b> <br>
                          Starts in Calendar Week:<b> %5</b> <br>
                          Meeting Description:<b> %6</b>")
                       .arg(meetingOccurrence)
                       .arg(onlineOfflineStatus)
                       .arg(inviteesNameEmail)
                       .arg(roomNumber)
                       .arg(calendarWeek)
                       .arg(meetingDescription))

            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            clip: true
            Accessible.readOnly: true
            Accessible.role: Accessible.StaticText
            Accessible.name: text
            Accessible.description: "A short summary of the meeting details"
            onVisibleChanged: {
                textSummary.text = qsTr((" Occurrence:<b> %1 </b> <br>
                                        Meeting to be held: <b>%2</b> <br>
                                        Invitees:<b> %3 </b> <br>
                                        Meeting Room: <b> %4 </b> <br>
                                        Starts in Calendar Week:<b> %5</b> <br>
                                        Meeting Description:<b> %6</b>").arg(meetingOccurrence).arg
                                        (onlineOfflineStatus).arg
                                        (inviteesNameEmail).arg(roomNumber).arg
                                        (calendarWeek).arg(meetingDescription))
                if (addedReadRequest == true) {
                    var temp = textSummary.text
                    textSummary.text = "<b>Read receipt requested<b><br>" + temp
                }
            }
        }
    }
}
