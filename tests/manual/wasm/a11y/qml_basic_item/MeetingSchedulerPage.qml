// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GroupBox {
    title: "Meeting Scheduler"
    property alias nextButton: nextButton
    property string meetingOccurrence: radioOnce.checked ? "Once" : "Weekly"
    property string onlineOfflineStatus: onlineMeeting.checked ? "Online" : "Offline"
    property int roomNumber: room.value
    property int calendarWeek: calendar.value
    property string meetingDescription: description.text
    property alias description: description
    height: parent.height-10
    ColumnLayout {
        anchors.fill: parent
        spacing: 10
        Row {
            spacing: 10
            RadioButton {
                id: radioOnce
                text: qsTr("Once")
                checked: true
                Accessible.role: Accessible.RadioButton
                Accessible.name: text
                Accessible.description: "Select this option if you want meeting once a week"
                Accessible.checkable: true

                Accessible.onToggleAction: {
                    toggle()
                }
            }
            RadioButton {
                id: radioWeekly
                text: qsTr("Weekly")
                Accessible.role: Accessible.RadioButton
                Accessible.name: text
                Accessible.description: "Select this option if you want meeting weekly"
                Accessible.checkable: true

                Accessible.onToggleAction: {
                    toggle()
                }
            }
        }

        CheckBox {
            id: onlineMeeting
            text: "Select if meeting will be online"
            Accessible.role: Accessible.CheckBox
            Accessible.name: text
            Accessible.description: "Select this option if you want an online meeting"
            Accessible.checkable: true

            Accessible.onToggleAction: {
                toggle()
            }
        }

        Row {
            spacing: 10
            enabled: onlineMeeting.checkState === Qt.Unchecked

            Label {
                text: "Select Meeting room"
                height: 50
                verticalAlignment: Text.AlignVCenter
                Accessible.role: Accessible.StaticText
                Accessible.name: text
                Accessible.description: "Select a meeting room"
            }

            SpinBox {
                id: room
                from: 0
                to: 10
                value: 0
                Accessible.role: Accessible.SpinBox
                Accessible.name: "Room number"
                Accessible.description: "Select a room for the meeting"
                Accessible.editable: true

                Accessible.onDecreaseAction: {
                    decrease()
                }
                Accessible.onIncreaseAction: {
                    increase()
                }
            }
        }

        Row {
            spacing: 10

            Label {
                text: "Calendar Week"
                height: 50
                verticalAlignment: Text.AlignVCenter
                Accessible.role: Accessible.StaticText
                Accessible.name: text
                Accessible.description: "Select the calendar week"
            }

            Slider {
                id: calendar
                from: 1
                to: 52
                Accessible.role: Accessible.Slider
                Accessible.name: "Calendar Week"
                Accessible.description: "Select the week"
                Accessible.onDecreaseAction: {
                    decrease()
                }
                Accessible.onIncreaseAction: {
                    increase()
                }
            }
        }

        Label {
            text: "Meeting Description"
            height: 50
            verticalAlignment: Text.AlignVCenter
            Accessible.role: Accessible.StaticText
            Accessible.name: text
            Accessible.description: "Meeting Description"
        }

        Rectangle {
            Layout.preferredHeight: 100
            Layout.preferredWidth: 310
            border.color: "black"
            border.width: 1
            ScrollView {
                id: view
                anchors.fill: parent
                clip: true

                TextArea {
                    id: description
                    wrapMode: TextEdit.Wrap
                    readOnly: false
                    font.pixelSize: 16
                    Accessible.role: Accessible.EditableText
                    Accessible.editable: true
                    Accessible.name: "Enter description"
                    Accessible.description: "Describe in short "
                    Accessible.multiLine: true
                }
            }
        }
        Button {
            id: nextButton
            text: "Next"
            Layout.alignment: Qt.AlignRight
            Accessible.role: Accessible.Button
            Accessible.name: text
            Accessible.description: "Press Button to go to next meeting Summary"
        }
    }
}
