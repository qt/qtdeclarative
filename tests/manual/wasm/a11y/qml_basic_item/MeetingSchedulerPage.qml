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
        spacing: 20
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
