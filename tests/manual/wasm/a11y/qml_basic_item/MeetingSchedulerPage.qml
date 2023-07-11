// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts


GroupBox {
    title: "Meeting Scheduler"
    property alias nextButton: nextButton

    ColumnLayout {
        anchors.fill: parent
        spacing: 10
        Row {
            spacing: 10
            RadioButton {
                text: qsTr("Once")
                Accessible.role: Accessible.RadioButton
                Accessible.name: text
                Accessible.description: "Select this option if you want meeting once a week"
                Accessible.checkable: true

                Accessible.onToggleAction: {
                    if (checkState === Qt.Checked)
                        checkState = Qt.Unchecked
                    else
                        checkState = Qt.Checked
                }
            }
            RadioButton {
                text: qsTr("Weekly")
                Accessible.role: Accessible.RadioButton
                Accessible.name: text
                Accessible.description: "Select this option if you want meeting weekly"
                Accessible.checkable: true

                Accessible.onToggleAction: {
                    if (checkState === Qt.Checked)
                        checkState = Qt.Unchecked
                    else
                        checkState = Qt.Checked
                }
            }

        }
        CheckBox {
            text: "Select if meeting will be online"
            Accessible.role: Accessible.CheckBox
            Accessible.name: text
            Accessible.description: "Select this option if you want an online meeting"
            Accessible.checkable: true

            Accessible.onToggleAction: {
                if (checkState === Qt.Checked)
                    checkState = Qt.Unchecked
                else
                    checkState = Qt.Checked
            }
        }

        Row {
            spacing: 10

            Label {
                text: "Select Meeting room"
                height: 50
                verticalAlignment: Text.AlignVCenter
                Accessible.role: Accessible.StaticText
                Accessible.name: text
                Accessible.description: "Select a meeting room"
            }

            SpinBox {
                from: 1
                to: 10
                value: 5

                Accessible.role: Accessible.SpinBox
                Accessible.name: "Room number"
                Accessible.description: "Select a room for the meeting"

                Accessible.onDecreaseAction: {
                    decrease()
                }
                Accessible.onIncreaseAction:  {
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
                from: 1
                to: 52

                Accessible.role: Accessible.Slider
                Accessible.name: "Calendar number"
                Accessible.description: "Select the week"
                Accessible.onDecreaseAction: {
                    decrease()
                }
                Accessible.onIncreaseAction:  {
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


            TextEdit {
                id: control
                anchors {
                    fill: parent
                    leftMargin: 5
                }

                wrapMode: TextEdit.Wrap
                Accessible.role: Accessible.EditableText
                Accessible.editable: true
                Accessible.name: "Enter descriptiom"
                Accessible.description: "Describe in short "
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
