// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ColumnLayout {
    property alias dialog: messageBox

    // Put it all in another ColumnLayout so we can easily add margins.
    ColumnLayout {
        Layout.leftMargin: 12
        Layout.rightMargin: 12
        Layout.topMargin: 12
        Layout.bottomMargin: 12

        GroupBox {
            title: qsTr("Dialog properties")

            Layout.fillWidth: true
            Layout.fillHeight: false

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
                        Layout.fillWidth: false

                        readonly property int modality: Qt.NonModal
                    }
                    RadioButton {
                        text: qsTr("Qt.WindowModal")
                        checked: true
                        Layout.fillWidth: false

                        readonly property int modality: Qt.WindowModal
                    }
                    RadioButton {
                        text: qsTr("Qt.ApplicationModal")
                        Layout.fillWidth: false

                        readonly property int modality: Qt.ApplicationModal
                    }
                }

                Label {
                    text: qsTr("result")
                }
                TextField {
                    id: resultTextField
                    text: messageBox.result === 1 ? qsTr("Accepted") : qsTr("Rejected")
                    readOnly: true
                    enabled: false
                    Layout.fillWidth: false
                }

                Label {
                    text: qsTr("title")
                }
                TextField {
                    id: titleTextField
                    text: qsTr("Hello World")
                    Layout.fillWidth: false
                }
            }
        }

        GroupBox {
            title: qsTr("MessageBox properties")

            Layout.fillWidth: true
            Layout.fillHeight: false

            ColumnLayout {
                RowLayout {
                    Label {
                        text: qsTr("text")
                    }
                    TextField {
                        id: textProperty
                        text: qsTr("default text")
                        Layout.fillWidth: false
                    }
                }
                RowLayout {
                    Label {
                        text: qsTr("informativeText")
                    }
                    TextField {
                        id: informativeTextProperty
                        text: qsTr("default informative text")
                        Layout.fillWidth: false
                    }
                }

                RowLayout {
                    Label {
                        text: qsTr("detailedText")
                    }
                    TextArea {
                        id: detailedTextProperty
                        background: Rectangle {
                            width: detailedTextProperty.width
                            height: detailedTextProperty.height
                            color: "white"
                            border.color: "black"
                            border.width: 1
                        }
                        Layout.maximumWidth: ApplicationWindow.window.width * 0.5
                        Layout.fillWidth: false
                        Layout.fillHeight: false
                        wrapMode: TextEdit.WordWrap
                        text: qsTr("This text will be displayed in the 'detailed text' textArea, which the user must press a button to see.\n"
                                 + "If this is an empty string, the button will not be visible.")
                    }
                }

                CheckBox {
                    id: okCheckbox
                    text: qsTr("Ok")
                    Layout.fillWidth: false

                    readonly property int value: checked ? MessageDialog.Ok : MessageDialog.NoButton
                }
                CheckBox {
                    id: saveCheckbox
                    text: qsTr("Save")
                    Layout.fillWidth: false

                    readonly property int value: checked ? MessageDialog.Save : MessageDialog.NoButton
                }
                CheckBox {
                    id: saveAllCheckbox
                    text: qsTr("Save All")
                    Layout.fillWidth: false

                    readonly property int value: checked ? MessageDialog.SaveAll : MessageDialog.NoButton
                }
                CheckBox {
                    id: openCheckbox
                    text: qsTr("Open")
                    Layout.fillWidth: false

                    readonly property int value: checked ? MessageDialog.Open : MessageDialog.NoButton
                }
                CheckBox {
                    id: yesCheckbox
                    text: qsTr("Yes")
                    Layout.fillWidth: false

                    readonly property int value: checked ? MessageDialog.Yes : MessageDialog.NoButton
                }
                CheckBox {
                    id: yesToAllCheckbox
                    text: qsTr("Yes to all")
                    Layout.fillWidth: false

                    readonly property int value: checked ? MessageDialog.YesToAll : MessageDialog.NoButton
                }
                CheckBox {
                    id: noCheckbox
                    text: qsTr("No")
                    Layout.fillWidth: false

                    readonly property int value: checked ? MessageDialog.No : MessageDialog.NoButton
                }
                CheckBox {
                    id: noToAllCheckbox
                    text: qsTr("No to all")
                    Layout.fillWidth: false

                    readonly property int value: checked ? MessageDialog.NoToAll : MessageDialog.NoButton
                }
                CheckBox {
                    id: abortCheckbox
                    text: qsTr("Abort")
                    Layout.fillWidth: false

                    readonly property int value: checked ? MessageDialog.Abort : MessageDialog.NoButton
                }
                CheckBox {
                    id: retryCheckbox
                    text: qsTr("Retry")
                    Layout.fillWidth: false

                    readonly property int value: checked ? MessageDialog.Retry : MessageDialog.NoButton
                }
                CheckBox {
                    id: ignoreCheckbox
                    text: qsTr("Ignore")
                    Layout.fillWidth: false

                    readonly property int value: checked ? MessageDialog.Ignore : MessageDialog.NoButton
                }
                CheckBox {
                    id: closeCheckbox
                    text: qsTr("Close")
                    Layout.fillWidth: false

                    readonly property int value: checked ? MessageDialog.Close : MessageDialog.NoButton
                }
                CheckBox {
                    id: cancelCheckbox
                    text: qsTr("Cancel")
                    Layout.fillWidth: false

                    readonly property int value: checked ? MessageDialog.Cancel : MessageDialog.NoButton
                }
                CheckBox {
                    id: discardCheckbox
                    text: qsTr("Discard")
                    Layout.fillWidth: false

                    readonly property int value: checked ? MessageDialog.Discard : MessageDialog.NoButton
                }
                CheckBox {
                    id: helpCheckbox
                    text: qsTr("Help")
                    Layout.fillWidth: false

                    readonly property int value: checked ? MessageDialog.Help : MessageDialog.NoButton
                }
                CheckBox {
                    id: applyCheckbox
                    text: qsTr("Apply")
                    Layout.fillWidth: false

                    readonly property int value: checked ? MessageDialog.Apply : MessageDialog.NoButton
                }
                CheckBox {
                    id: resetCheckbox
                    text: qsTr("Reset")
                    Layout.fillWidth: false

                    readonly property int value: checked ? MessageDialog.Reset : MessageDialog.NoButton
                }
            }
        }

        MessageDialog {
            id: messageBox
            buttons: okCheckbox.value |
                     saveCheckbox.value |
                     saveAllCheckbox.value |
                     openCheckbox.value |
                     yesCheckbox.value |
                     yesToAllCheckbox.value |
                     noCheckbox.value |
                     noToAllCheckbox.value |
                     abortCheckbox.value |
                     retryCheckbox.value |
                     ignoreCheckbox.value |
                     closeCheckbox.value |
                     cancelCheckbox.value |
                     discardCheckbox.value |
                     helpCheckbox.value |
                     applyCheckbox.value |
                     resetCheckbox.value
            modality: modalityButtonGroup.checkedButton.modality
            title: titleTextField.text
            text: textProperty.text
            informativeText: informativeTextProperty.text
            detailedText: detailedTextProperty.text
            onButtonClicked: function(button, role) {
                if (button & MessageDialog.Ok)
                    console.log("Ok pressed")
                else if (button & MessageDialog.Save)
                    console.log("Save pressed")
                else if (button & MessageDialog.SaveAll)
                    console.log("Save to all pressed")
                else if (button & MessageDialog.Open)
                    console.log("Open pressed")
                else if (button & MessageDialog.Yes)
                    console.log("Yes pressed")
                else if (button & MessageDialog.YesToAll)
                    console.log("Yes to all pressed")
                else if (button & MessageDialog.No)
                    console.log("No pressed")
                else if (button & MessageDialog.NoToAll)
                    console.log("No to all pressed")
                else if (button & MessageDialog.Abort)
                    console.log("Abort pressed")
                else if (button & MessageDialog.Retry)
                    console.log("Retry pressed")
                else if (button & MessageDialog.Ignore)
                    console.log("Ignore pressed")
                else if (button & MessageDialog.Close)
                    console.log("Close pressed")
                else if (button & MessageDialog.Cancel)
                    console.log("Cancel pressed")
                else if (button & MessageDialog.Discard)
                    console.log("Discard pressed")
                else if (button & MessageDialog.Help)
                    console.log("Help pressed")
                else if (button & MessageDialog.Apply)
                    console.log("Apply pressed")
                else if (button & MessageDialog.Reset)
                    console.log("Rest pressed")
            }
        }
    }
}
