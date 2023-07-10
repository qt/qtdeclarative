// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

GroupBox {
    title: "Add Invitees"
    property alias nextButton: nextButton
    ColumnLayout {
        anchors.fill: parent
        spacing: 20

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

