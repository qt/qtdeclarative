/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
                    text: messageBox.result === 1 ? qsTr("Accepted") : qsTr("Rejected")
                    readOnly: true
                    enabled: false
                }

                Label {
                    text: qsTr("title")
                }
                TextField {
                    id: titleTextField
                    text: qsTr("Hello World")
                }
            }
        }

        GroupBox {
            title: qsTr("MessageBox properties")

            Layout.fillWidth: true

            ColumnLayout {
                RowLayout {
                    Label {
                        text: qsTr("text")
                    }
                    TextField {
                        id: textProperty
                        text: qsTr("default text")
                    }
                }
                RowLayout {
                    Label {
                        text: qsTr("informativeText")
                    }
                    TextField {
                        id: informativeTextProperty
                        text: qsTr("default informative text")
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
                        wrapMode: TextEdit.WordWrap
                        text: qsTr("This text will be displayed in the 'detailed text' textArea, which the user must press a button to see.\n"
                                 + "If this is an empty string, the button will not be visible.")
                    }
                }

                CheckBox {
                    id: okCheckbox
                    text: qsTr("Ok")

                    readonly property int value: checked ? MessageDialog.Ok : MessageDialog.NoButton
                }
                CheckBox {
                    id: saveCheckbox
                    text: qsTr("Save")

                    readonly property int value: checked ? MessageDialog.Save : MessageDialog.NoButton
                }
                CheckBox {
                    id: saveAllCheckbox
                    text: qsTr("Save All")

                    readonly property int value: checked ? MessageDialog.SaveAll : MessageDialog.NoButton
                }
                CheckBox {
                    id: openCheckbox
                    text: qsTr("Open")

                    readonly property int value: checked ? MessageDialog.Open : MessageDialog.NoButton
                }
                CheckBox {
                    id: yesCheckbox
                    text: qsTr("Yes")

                    readonly property int value: checked ? MessageDialog.Yes : MessageDialog.NoButton
                }
                CheckBox {
                    id: yesToAllCheckbox
                    text: qsTr("Yes to all")

                    readonly property int value: checked ? MessageDialog.YesToAll : MessageDialog.NoButton
                }
                CheckBox {
                    id: noCheckbox
                    text: qsTr("No")

                    readonly property int value: checked ? MessageDialog.No : MessageDialog.NoButton
                }
                CheckBox {
                    id: noToAllCheckbox
                    text: qsTr("No to all")

                    readonly property int value: checked ? MessageDialog.NoToAll : MessageDialog.NoButton
                }
                CheckBox {
                    id: abortCheckbox
                    text: qsTr("Abort")

                    readonly property int value: checked ? MessageDialog.Abort : MessageDialog.NoButton
                }
                CheckBox {
                    id: retryCheckbox
                    text: qsTr("Retry")

                    readonly property int value: checked ? MessageDialog.Retry : MessageDialog.NoButton
                }
                CheckBox {
                    id: ignoreCheckbox
                    text: qsTr("Ignore")

                    readonly property int value: checked ? MessageDialog.Ignore : MessageDialog.NoButton
                }
                CheckBox {
                    id: closeCheckbox
                    text: qsTr("Close")

                    readonly property int value: checked ? MessageDialog.Close : MessageDialog.NoButton
                }
                CheckBox {
                    id: cancelCheckbox
                    text: qsTr("Cancel")

                    readonly property int value: checked ? MessageDialog.Cancel : MessageDialog.NoButton
                }
                CheckBox {
                    id: discardCheckbox
                    text: qsTr("Discard")

                    readonly property int value: checked ? MessageDialog.Discard : MessageDialog.NoButton
                }
                CheckBox {
                    id: helpCheckbox
                    text: qsTr("Help")

                    readonly property int value: checked ? MessageDialog.Help : MessageDialog.NoButton
                }
                CheckBox {
                    id: applyCheckbox
                    text: qsTr("Apply")

                    readonly property int value: checked ? MessageDialog.Apply : MessageDialog.NoButton
                }
                CheckBox {
                    id: resetCheckbox
                    text: qsTr("Reset")

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
