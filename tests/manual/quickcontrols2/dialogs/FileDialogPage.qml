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
                    text: fileDialog.result === 1 ? qsTr("Accepted") : qsTr("Rejected")
                    readOnly: true
                    enabled: false
                }

                Label {
                    text: qsTr("title")
                }
                TextField {
                    id: titleTextField
                    text: qsTr("Choose a file")
                }
            }
        }

        GroupBox {
            title: qsTr("FileDialog properties")

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
                    text: qsTr("fileOptions")

                    Layout.alignment: Qt.AlignTop
                }
                ColumnLayout {
                    id: fileOptionsColumnLayout

                    CheckBox {
                        id: dontResolveSymlinksCheckBox
                        text: qsTr("DontResolveSymlinks")

                        readonly property int fileOption: checked ? FileDialog.DontResolveSymlinks : 0
                    }
                    CheckBox {
                        id: dontConfirmOverwriteCheckBox
                        text: qsTr("DontConfirmOverwrite")

                        readonly property int fileOption: checked ? FileDialog.DontConfirmOverwrite : 0
                    }
                    CheckBox {
                        id: readOnlyCheckBox
                        text: qsTr("ReadOnly")

                        readonly property int fileOption: checked ? FileDialog.ReadOnly : 0
                    }
                    CheckBox {
                        id: hideNameFilterDetailsCheckBox
                        text: qsTr("HideNameFilterDetails")

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

                        readonly property int fileMode: FileDialog.OpenFile
                    }
                    RadioButton {
                        text: qsTr("OpenFiles")
                        checked: true

                        readonly property int fileMode: FileDialog.OpenFiles
                    }
                    RadioButton {
                        text: qsTr("SaveFile")

                        readonly property int fileMode: FileDialog.SaveFile
                    }
                }

                Label {
                    text: qsTr("nameFilters")
                }
                TextField {
                    id: nameFiltersTextField
                    text: ["Text files (*.txt)", "HTML files (*.html)"].join(",")

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
                }

                Label {
                    text: qsTr("selectedFile")
                }
                TextField {
                    id: selectedFileTextField
                    text: fileDialog.selectedFile
                    readOnly: true
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
        }
    }
}
