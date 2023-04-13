// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [file]
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

ApplicationWindow {
    id: window
    width: 600
    height: 600
    visible: true
    title: qsTr("Time Picker Example")

    Material.theme: darkThemeSwitch.checked ? Material.Dark : Material.Light

    // Shows the selected time and opens the dialog.
    TimeComponentLabel {
        id: openDialogLabel
        width: parent.width - 80
        anchors.centerIn: parent
        font.pixelSize: Qt.application.font.pixelSize * 8
        renderTypeQuality: Text.VeryHighRenderTypeQuality
        interactive: !timePickerDialog.opened

        text: Qt.formatTime(new Date(1970, 1, 1, timePickerDialog.hours, timePickerDialog.minutes), "hh:mm")

        onTapped: timePickerDialog.openWithMode(TimePicker.Mode.Hours)
    }

    ColumnLayout {
        // We always want the openDialogLabel to be centered in the window, not us.
        // For that reason, we use anchors rather than putting the root items into a ColumnLayout.
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: openDialogLabel.bottom
        anchors.topMargin: 24
        spacing: 12

        Switch {
            id: is24HourSwitch
            text: qsTr("24 Hour")
            checked: timePickerDialog.is24Hour
        }
        Switch {
            id: darkThemeSwitch
            text: qsTr("Dark")
        }
    }

    TimePickerDialog {
        id: timePickerDialog
        anchors.centerIn: parent
        is24Hour: is24HourSwitch.checked

        onTimeAccepted: print("A time was chosen - do something here!")
    }
}
//! [file]
