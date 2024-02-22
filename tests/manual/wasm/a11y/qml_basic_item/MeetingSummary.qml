// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GroupBox {
    id: root
    title: "Summary"
    height: parent.height-10
    property string meetingOccurrence: "Once"
    property string onlineOfflineStatus: "offline"
    property int roomNumber: 0
    property int calendarWeek: 1
    property string meetingDescription: "No Description"

    property string inviteesNameEmail: ""
    property bool addedReadRequest: false
    clip: true

    Flickable {
        id: flickable
        width: parent.width - 10
        height: parent.height - 10
        contentHeight: meetingHeader.contentHeight + textSummary.contentHeight + 50
        contentWidth: parent.width - 10
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AlwaysOn

            Accessible.role: Accessible.ScrollBar
            Accessible.name: "Vertical ScrollBar"
            Accessible.description: "Use this to scroll summary page"
            Accessible.onDecreaseAction: {
                decrease()
            }
            Accessible.onIncreaseAction: {
                increase()
            }
        }

        Label {
            id: meetingHeader
            height: 30
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
        TextEdit {
            id: textSummary
            anchors {
                left: parent.left
                top: meetingHeader.bottom
                topMargin: 10
                leftMargin: 10
            }
            width: parent.width - 50
            font.pixelSize: 16
            textFormat: TextEdit.RichText
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
            readOnly: true
            clip: true
            Accessible.readOnly: readOnly
            Accessible.role: Accessible.StaticText
            Accessible.name: textSummary.getText(0, textSummary.length)
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
