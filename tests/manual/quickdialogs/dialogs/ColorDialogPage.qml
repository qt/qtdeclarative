// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ColumnLayout {
    property alias dialog: colorDialog

    // Put it all in another ColumnLayout so we can easily add margins.
    ColumnLayout {
        Layout.leftMargin: 12
        Layout.rightMargin: 12
        Layout.topMargin: 12
        Layout.bottomMargin: 12

        GroupBox {
            title: qsTr("Dialog properties")

            Layout.fillWidth: true

            GridLayout {
                columns: 2
                anchors.fill: parent

                Label {
                    text: qsTr("modality")

                    Layout.alignment: Qt.AlignTop
                    Layout.minimumWidth: ApplicationWindow.window.width * 0.2
                    Layout.maximumWidth: ApplicationWindow.window.width * 0.2
                }
                ButtonGroup {
                    id: modalityButtonGroup
                    buttons: modalityColumnLayout.children
                }
                ColumnLayout {
                    id: modalityColumnLayout

                    RadioButton {
                        text: qsTr("Qt.NonModal")

                        readonly property int modality: Qt.NonModal
                    }
                    RadioButton {
                        text: qsTr("Qt.WindowModal")
                        checked: true

                        readonly property int modality: Qt.WindowModal
                    }
                    RadioButton {
                        text: qsTr("Qt.ApplicationModal")

                        readonly property int modality: Qt.ApplicationModal
                    }
                }

                Label {
                    text: qsTr("result")
                }
                TextField {
                    id: resultTextField
                    text: colorDialog.result === 1 ? qsTr("Accepted") : qsTr("Rejected")
                    readOnly: true
                    enabled: false
                }

                Label {
                    text: qsTr("title")
                }
                TextField {
                    id: titleTextField
                    text: qsTr("A Color Dialog")
                }
            }
        }

        GroupBox {
            title: qsTr("ColorDialog properties")

            Layout.fillWidth: true

            GridLayout {
                columns: 2
                anchors.fill: parent

                Label {
                    text: qsTr("selectedColor (code)")
                }

                TextField {
                    id: selectedColorTextField
                    text: colorDialog.selectedColor
                    selectByMouse: true
                    onEditingFinished: function() { colorDialog.selectedColor = text; }

                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("selectedColor")
                }

                Rectangle {
                    id: selectedColorRect
                    color: colorDialog.selectedColor
                    implicitWidth: 100
                    implicitHeight: 100
                }

                Label {
                    text: qsTr("options")

                    Layout.alignment: Qt.AlignTop
                }

                ColumnLayout {
                    id: colorOptionsColumnLayout

                    CheckBox {
                        id: showAlphaChannel
                        text: qsTr("ShowAlphaChannel")

                        readonly property int colorOption: checked ? ColorDialog.ShowAlphaChannel : 0
                    }
                    CheckBox {
                        id: noButtons
                        text: qsTr("NoButtons")

                        readonly property int colorOption: checked ? ColorDialog.NoButtons : 0
                    }
                }
            }
        }

        ColorDialog {
            id: colorDialog
            modality: modalityButtonGroup.checkedButton.modality
            title: titleTextField.text
            options: showAlphaChannel.colorOption | noButtons.colorOption
        }
    }
}
