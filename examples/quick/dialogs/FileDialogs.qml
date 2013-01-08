/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
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

import QtQuick 2.0
import QtQuick.Dialogs 1.0
import "../shared"

Rectangle {

    width: 580
    height: 360
    color: "#444444"

    Rectangle {
        id: toolbar
        width: parent.width
        height: 40
        border.color: "black"
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#444444" }
            GradientStop { position: 0.3; color: "grey" }
            GradientStop { position: 0.85; color: "grey" }
            GradientStop { position: 1.0; color: "white" }
        }
        Row {
            spacing: 6
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 8
            height: parent.height - 6
            width: parent.width
            ToolButton {
                text: "Open"
                anchors.verticalCenter: parent.verticalCenter
                onClicked: fileDialog.open()
            }
            ToolButton {
                text: "Close"
                anchors.verticalCenter: parent.verticalCenter
                onClicked: fileDialog.close()
            }
            ToolButton {
                text: "/tmp"
                anchors.verticalCenter: parent.verticalCenter
                // TODO: QTBUG-29814 This isn't portable, but we don't expose QDir::tempPath to QML yet.
                onClicked: fileDialog.folder = "/tmp"
            }
        }
    }

    FileDialog {
        id: fileDialog
        visible: fileDialogVisible.checked
        modality: fileDialogModal.checked ? Qt.WindowModal : Qt.NonModal
        title: fileDialogSelectFolder.checked ? "Choose a folder" :
            (fileDialogSelectMultiple.checked ? "Choose some files" : "Choose a file")
        selectExisting: fileDialogSelectExisting.checked
        selectMultiple: fileDialogSelectMultiple.checked
        selectFolder: fileDialogSelectFolder.checked
        nameFilters: [ "Image files (*.png *.jpg)", "All files (*)" ]
        selectedNameFilter: "All files (*)"
        onAccepted: { console.log("Accepted: " + fileUrls) }
        onRejected: { console.log("Rejected") }
    }

    Column {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: toolbar.bottom
        anchors.margins: 8
        spacing: 8
        Text {
            color: "white"
            font.bold: true
            text: "File dialog properties:"
        }
        CheckBox {
            id: fileDialogModal
            text: "Modal"
            checked: true
            Binding on checked { value: fileDialog.modality != Qt.NonModal }
        }
        CheckBox {
            id: fileDialogSelectFolder
            text: "Select Folder"
            Binding on checked { value: fileDialog.selectFolder }
        }
        CheckBox {
            id: fileDialogSelectExisting
            text: "Select Existing Files"
            checked: true
            Binding on checked { value: fileDialog.selectExisting }
        }
        CheckBox {
            id: fileDialogSelectMultiple
            text: "Select Multiple Files"
            Binding on checked { value: fileDialog.selectMultiple }
        }
        CheckBox {
            id: fileDialogVisible
            text: "Visible"
            Binding on checked { value: fileDialog.visible }
        }
        Text {
            color: "#EEEEDD"
            text: "<b>current view folder:</b> " + fileDialog.folder
        }
        Text {
            color: "#EEEEDD"
            text: "<b>name filters:</b> {" + fileDialog.nameFilters + "}; current filter: " + fileDialog.selectedNameFilter
            width: parent.width
            wrapMode: Text.Wrap
        }
        Text {
            color: "#EEEEDD"
            text: "<b>chosen files:</b> " + fileDialog.fileUrls
            width: parent.width
            wrapMode: Text.Wrap
        }
        Text {
            color: "#EEEEDD"
            text: "<b>chosen single path:</b> " + fileDialog.fileUrl
            width: parent.width
            wrapMode: Text.Wrap
        }
    }
}
