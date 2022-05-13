// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import "."

ColumnLayout {
    property alias dialog: folderDialog

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
                    text: folderDialog.result === 1 ? qsTr("Accepted") : qsTr("Rejected")
                    readOnly: true
                    enabled: false
                }

                Label {
                    text: qsTr("title")
                }
                TextField {
                    id: titleTextField
                    text: qsTr("Choose a folder")
                }
            }
        }

        GroupBox {
            title: qsTr("FolderDialog properties")

            Layout.fillWidth: true

            GridLayout {
                columns: 2
                anchors.fill: parent

                Label {
                    text: qsTr("acceptLabel")

                    Layout.minimumWidth: ApplicationWindow.window.width * 0.2
                    Layout.maximumWidth: ApplicationWindow.window.width * 0.2
                }
                TextField {
                    id: acceptLabelTextField
                    text: qsTr("OK")
                }

                Label {
                    text: qsTr("rejectLabel")
                }
                TextField {
                    id: rejectLabelTextField
                    text: qsTr("Cancel")
                }

                Label {
                    text: qsTr("currentFolder")
                }
                TextField {
                    id: currentFolderTextField
                    text: folderDialog.currentFolder
                    readOnly: true
                    selectByMouse: true

                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("options")

                    Layout.alignment: Qt.AlignTop
                }
                ColumnLayout {
                    id: folderOptionsColumnLayout

                    CheckBox {
                        id: dontResolveSymlinksCheckBox
                        text: qsTr("DontResolveSymlinks")

                        readonly property int folderOption: checked ? FolderDialog.DontResolveSymlinks : 0
                    }
                    CheckBox {
                        id: readOnlyCheckBox
                        text: qsTr("ReadOnly")

                        readonly property int folderOption: checked ? FolderDialog.ReadOnly : 0
                    }
                }

                Label {
                    text: qsTr("selectedFolder")
                }
                TextField {
                    text: folderDialog.selectedFolder
                    readOnly: true
                    selectByMouse: true

                    Layout.fillWidth: true
                }
            }
        }

        FolderDialog {
            id: folderDialog

            modality: modalityButtonGroup.checkedButton.modality
            title: titleTextField.text

            acceptLabel: acceptLabelTextField.text
            options: dontResolveSymlinksCheckBox.folderOption | readOnlyCheckBox.folderOption
            rejectLabel: rejectLabelTextField.text
        }
    }
}
