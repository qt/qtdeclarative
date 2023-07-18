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

    ColumnLayout{
        width: 300
        height: 200
        spacing: 2
        Label{
            Layout.preferredHeight: 30
            text: "Meeting Schedule"
            font.bold: true
            font.pixelSize: 24
            Accessible.role: Accessible.StaticText
            Accessible.name: text
            Accessible.description: "Title"
        }

        Text {
            Layout.alignment: Qt.AlignLeft
            Layout.leftMargin: 10
            Layout.preferredWidth: 300
            font.pixelSize: 16
            textFormat: Text.StyledText
            text: qsTr((" Occurrence:<b> %1 </b> <br>
            Meeting to be held: <b>%2</b> <br>
            Meeting Room: <b> %3 </b> <br>
            Starts in Calendar Week:<b> %4</b> <br>
            Meeting Description:<b> %5</b>")
                       .arg(meetingOccurrence)
                       .arg(onlineOfflineStatus)
                       .arg(roomNumber)
                       .arg(calendarWeek)
                       .arg(meetingDescription))

            wrapMode: Text.WrapAtWordBoundaryOrAnywhere

            Accessible.readOnly: true
            Accessible.role: Accessible.StaticText
            Accessible.name: text
            Accessible.description: "A short summary of the meeting details"
        }

    }
}
