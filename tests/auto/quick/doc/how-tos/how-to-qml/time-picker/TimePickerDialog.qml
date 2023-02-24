// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [file]
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

Dialog {
    id: root
    standardButtons: Dialog.Ok | Dialog.Cancel
    closePolicy: Dialog.NoAutoClose
    // We don't want so much space between the picker and dialog buttons.
    bottomPadding: 8

    property int hours: 12
    property int minutes: 0
    property alias is24Hour: timePicker.is24Hour

    property int __initialHours
    property int __initialMinutes

    signal timeAccepted
    signal timeRejected

    function openWithMode(mode) {
        timePicker.openWith(mode !== undefined ? mode : TimePicker.Mode.Hours, hours, minutes)

        __initialHours = hours
        __initialMinutes = minutes

        open()
    }

    onAccepted: {
        root.hours = timePicker.hours
        root.minutes = timePicker.minutes
        root.timeAccepted()
    }
    onRejected: {
        hours = __initialHours
        minutes = __initialMinutes
        // Also reset the picker's time so that the onIs24HourChanged handler below works as expected.
        timePicker.hours = __initialHours
        timePicker.minutes = __initialMinutes
        root.timeRejected()
    }

    // If is24Hour changes programmatically (only while we're not open),
    // make sure we adapt to any possible clamping it did in the transition from 24 hours to 12.
    onIs24HourChanged: {
        if (!opened)
            root.hours = timePicker.hours
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        TimePickerLabel {
            id: timeLabel
            // Use TimePicker's time, because that is updated live, whereas our values
            // are only changed once we've been accepted.
            time: new Date(1970, 1, 1, timePicker.hours, timePicker.minutes)
            hoursActive: timePicker.mode === TimePicker.Mode.Hours
            showAmPm: !timePicker.is24Hour

            Layout.fillWidth: true
            // Push us down a bit so we're not so close to the top of the dialog.
            Layout.topMargin: 8

            onHoursSelected: timePicker.mode = TimePicker.Mode.Hours
            onMinutesSelected: timePicker.mode = TimePicker.Mode.Minutes
        }

        TimePicker {
            id: timePicker
            objectName: "timePicker"
            // Our TapHandler may handle the click event on the Label if we don't do this,
            // causing an hour to be inadvertently selected.
            interactive: root.opened

            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
//! [file]
