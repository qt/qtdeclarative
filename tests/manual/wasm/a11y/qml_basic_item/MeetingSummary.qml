// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GroupBox {
    id: root
    title: "Summary"
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
        width: 500
        height: 450
        contentHeight: meetingHeader.contentHeight + textSummary.contentHeight + 50
        contentWidth: 500
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
        Text {
            id: textSummary
            anchors {
                left: parent.left
                top: meetingHeader.bottom
                topMargin: 10
                leftMargin: 10
            }
            width: 300
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
