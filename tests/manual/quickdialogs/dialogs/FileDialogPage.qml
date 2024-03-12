// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import "."

ColumnLayout {
    property alias dialog: fileDialog

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
                    text: fileDialog.result === 1 ? qsTr("Accepted") : qsTr("Rejected")
                    readOnly: true
                    enabled: false
                    Layout.fillWidth: false
                }

                Label {
                    text: qsTr("title")
                }
                TextField {
                    id: titleTextField
                    text: qsTr("Choose a file")
                    Layout.fillWidth: false
                }
            }
        }

        GroupBox {
            title: qsTr("FileDialog properties")

            Layout.fillWidth: true
            Layout.fillHeight: false

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
                    Layout.fillWidth: false
                }

                Label {
                    text: qsTr("currentFile")
                }
                TextField {
                    id: currentFileTextField
                    text: fileDialog.currentFile
                    readOnly: true
                    selectByMouse: true

                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("currentFolder")
                }
                TextField {
                    id: currentFolderTextField
                    text: fileDialog.currentFolder
                    readOnly: true
                    selectByMouse: true

                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("currentFiles")

                    Layout.alignment: Qt.AlignTop
                }
                StringListView {
                    id: currentFilesListView
                    // QTBUG-72906
                    model: [].concat(fileDialog.currentFiles)
                }

                Label {
                    text: qsTr("options")

                    Layout.alignment: Qt.AlignTop
                }
                ColumnLayout {
                    id: fileOptionsColumnLayout

                    CheckBox {
                        id: dontResolveSymlinksCheckBox
                        text: qsTr("DontResolveSymlinks")
                        Layout.fillWidth: false

                        readonly property int fileOption: checked ? FileDialog.DontResolveSymlinks : 0
                    }
                    CheckBox {
                        id: dontConfirmOverwriteCheckBox
                        text: qsTr("DontConfirmOverwrite")
                        Layout.fillWidth: false

                        readonly property int fileOption: checked ? FileDialog.DontConfirmOverwrite : 0
                    }
                    CheckBox {
                        id: readOnlyCheckBox
                        text: qsTr("ReadOnly")
                        Layout.fillWidth: false

                        readonly property int fileOption: checked ? FileDialog.ReadOnly : 0
                    }
                    CheckBox {
                        id: hideNameFilterDetailsCheckBox
                        text: qsTr("HideNameFilterDetails")
                        Layout.fillWidth: false

                        readonly property int fileOption: checked ? FileDialog.HideNameFilterDetails : 0
                    }
                }

                Label {
                    text: qsTr("fileMode")

                    Layout.alignment: Qt.AlignTop
                }
                ButtonGroup {
                    id: fileModeButtonGroup
                    buttons: fileModeColumnLayout.children
                }
                ColumnLayout {
                    id: fileModeColumnLayout

                    RadioButton {
                        text: qsTr("OpenFile")
                        Layout.fillWidth: false

                        readonly property int fileMode: FileDialog.OpenFile
                    }
                    RadioButton {
                        text: qsTr("OpenFiles")
                        checked: true
                        Layout.fillWidth: false

                        readonly property int fileMode: FileDialog.OpenFiles
                    }
                    RadioButton {
                        text: qsTr("SaveFile")
                        Layout.fillWidth: false

                        readonly property int fileMode: FileDialog.SaveFile
                    }
                }

                Label {
                    text: qsTr("nameFilters")
                }
                TextField {
                    id: nameFiltersTextField
                    text: ["Text files (*.txt)", "HTML files (*.html), Images (*.jpg *.png *.svg)"].join(",")

                    Layout.fillWidth: true

                    ToolTip.text: qsTr("For this example, a comma-separated string")
                    ToolTip.visible: hovered
                    ToolTip.delay: Theme.toolTipDelay
                }

                Label {
                    text: qsTr("rejectLabel")
                }
                TextField {
                    id: rejectLabelTextField
                    text: qsTr("Cancel")
                    Layout.fillWidth: false
                }

                Label {
                    text: qsTr("selectedFile")
                }
                TextField {
                    id: selectedFileTextField
                    text: fileDialog.selectedFile
                    selectByMouse: true

                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("selectedFiles")

                    Layout.alignment: Qt.AlignTop
                }
                StringListView {
                    id: selectedFilesListView
                    // QTBUG-72906
                    model: [].concat(fileDialog.selectedFiles)
                }

                Label {
                    text: qsTr("selectedNameFilter.name")
                }
                TextField {
                    id: selectedNameFilterNameTextField
                    text: fileDialog.selectedNameFilter.name
                    readOnly: true
                    selectByMouse: true

                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("selectedNameFilter.globs")

                    Layout.alignment: Qt.AlignTop
                }
                StringListView {
                    id: selectedNameFilterGlobsListView
                    // QTBUG-72906
                    model: [].concat(fileDialog.selectedNameFilter.globs)
                }

                Label {
                    text: qsTr("selectedNameFilter.index")
                }
                TextField {
                    id: selectedNameFilterIndexTextField
                    text: fileDialog.selectedNameFilter.index
                    readOnly: true
                    selectByMouse: true

                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("selectedNameFilter.extensions")

                    Layout.alignment: Qt.AlignTop
                }
                StringListView {
                    id: selectedNameFilterExtensionsListView
                    // QTBUG-72906
                    model: [].concat(fileDialog.selectedNameFilter.extensions)
                }
            }
        }

        FileDialog {
            id: fileDialog

            modality: modalityButtonGroup.checkedButton.modality
            title: titleTextField.text

            acceptLabel: acceptLabelTextField.text
            fileMode: fileModeButtonGroup.checkedButton.fileMode
            options: dontResolveSymlinksCheckBox.fileOption
                | dontConfirmOverwriteCheckBox.fileOption
                | readOnlyCheckBox.fileOption
                | hideNameFilterDetailsCheckBox.fileOption
            nameFilters: nameFiltersTextField.text.split(",")
            rejectLabel: rejectLabelTextField.text
            selectedFile: selectedFileTextField.text
        }
    }
}
