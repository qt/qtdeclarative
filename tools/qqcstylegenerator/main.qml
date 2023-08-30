// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtCore

ApplicationWindow {
    id: window
    width: 640
    height: mainColumn.height + (mainColumn.anchors.margins * 2)
    visible: true
    title: "Qt Figma Style Generator"

    property bool closeRequested: false
    property bool generating: false
    property bool stopping: false
    property string figmaFileName: ""

    onClosing: (closeEvent) => {
        if (generating) {
            closeRequested = true
            stop()
            closeEvent.accepted = false
        }
    }

    ListModel {
        id: outputModel
    }

    ListModel {
        id: filteredModel
    }

    Connections {
        target: bridge

        function onDebug(msg) {
            output(msg)
        }

        function onWarning(msg) {
            if (!outputScrollView.visible)
                outputFilter.text = "(warning|error)"
            updateOutputFilter();
            output("Warning: " + msg)
            showLogCheckbox.checked = true
        }

        function onError(msg) {
            if (!outputScrollView.visible)
                outputFilter.text = "(warning|error)"
            updateOutputFilter();
            output("Error: " + msg)
            showLogCheckbox.checked = true
        }

        function onStarted() {
            generating = true
        }

        function onFinished() {
            progressBar.value = 0
            progressBar.indeterminate = false
            statusLabel.text = stopping ? "Stopped!" : "Finished!"
            output(stopping ? "Stopped!" : "Finished!")
            stopping = false
            generating = false
            if (closeRequested)
                window.close()
        }

        function onProgressToChanged(to) {
            if (stopping)
                return
            progressBar.to = to
            progressBar.value = 0
            progressBar.indeterminate = to === 0
        }

        function onProgressLabelChanged(label) {
            if (stopping)
                return
            statusLabel.text = label
            output(label)
        }

        function onProgress() {
            progressBar.value++
        }

        function onFigmaFileNameChanged(name) {
            figmaFileName = name
            output("Figma name: " + name)
        }
    }

    function output(msg)
    {
        outputModel.append({"msg": msg})
        const regex = new RegExp(outputFilter.text, "i");
        if (regex.test(msg))
            filteredModel.append({"msg" : msg})
    }

    function updateOutputFilter() {
        filteredModel.clear()
        const regex = new RegExp(outputFilter.text, "i");
        let rows = outputModel.rowCount()
        for (let row = 0; row < rows; ++row) {
            let index = outputModel.index(row, 0)
            let msg = outputModel.data(index)
            if (regex.test(msg))
                filteredModel.append({"msg" : msg})
        }
    }

    function stop()
    {
        stopping = true
        statusLabel.text = "Stopping..."
        bridge.stop()
    }

    ColumnLayout {
        id: mainColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 30
        spacing: 20

        Image {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: 50
            source: Qt.resolvedUrl("background.png")
            Image {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.margins: 10
                source: Qt.resolvedUrl("figma_and_qt.png")
                fillMode: Image.PreserveAspectFit
            }
        }

        GridLayout {
            width: parent.width
            columns: 2

            Label {
                text: "Target directory"
            }

            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: stylePathInput
                    placeholderText: "The path to where you wish to save the style"
                    Layout.fillWidth: true
                    text: bridge.targetDirectory
                    onTextChanged: bridge.targetDirectory = text
                }

                ToolButton {
                    icon.source: "qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/folder-icon-round.png"
                    onClicked: stylePathDialog.open()
                }

                FolderDialog {
                    id: stylePathDialog
                    onAccepted: stylePathInput.text = bridge.toLocalFile(selectedFolder)
                }
            }

            Label {
                text: "Figma URL (or file ID)"
            }

            TextField {
                placeholderText: "URL / file ID"
                Layout.fillWidth: true
                text: bridge.figmaUrlOrId
                onTextChanged: bridge.figmaUrlOrId = text
                ToolTip.visible: hovered
                ToolTip.delay: 1000
                ToolTip.text: "How to get the Figma file ID:"
                              + "\n1. Click the 'Share' button in the top right corner in Figma"
                              + "\n2. Click 'Copy link'"
            }

            Label {
                text: "Figma Token"
            }

            TextField {
                placeholderText: "Token"
                Layout.fillWidth: true
                text: bridge.figmaToken
                onTextChanged: bridge.figmaToken = text
                ToolTip.visible: hovered
                ToolTip.delay: 1000
                ToolTip.text: "How to get a Figma token:"
                              + "\n1. Login to your Figma account."
                              + "\n2. Head to Settings from the top-left menu inside Figma."
                              + "\n3. Find the Personal access tokens section."
                              + "\n4. Click Generate new token to open the configuration modal."
                              + "\n5. Click Generate token."
            }

            Label {
                text: "Overwrite QML"
            }

            CheckBox {
                checked: bridge.overwriteQml
                onCheckedChanged: bridge.overwriteQml = checked
            }

            Label {
                text: "Show log"
            }

            CheckBox {
                id: showLogCheckbox
                checked: false
                onCheckedChanged: outputScrollView.visible = checked
            }

            Label {
                text: "Show usage"
            }

            CheckBox {
                id: usageCheckbox
                checked: false
            }
        }

        Frame {
            visible: usageCheckbox.checked
            Layout.fillWidth: true
            Layout.columnSpan: 2
            TextArea {
                anchors.fill: parent
                readonly property int slashIndex: bridge.targetDirectory.lastIndexOf("/")
                readonly property string importPath: bridge.targetDirectory.slice(0, slashIndex)
                readonly property string styleName: bridge.targetDirectory.slice(slashIndex + 1)
                readOnly: true
                wrapMode: TextEdit.Wrap
                text: slashIndex === -1 || importPath === "" || styleName === ""
                      ? "You need to set a valid target directory!"
                      : "The name of this style will be '" + styleName + "', and the import path"
                      + " will be '" + importPath + "'"
                      + "\n\nIf you assign the import path to the environment variable 'QML_IMPORT_PATH'"
                      + "\nan application can use this style by for example launching it with the name of"
                      + " the style as argument:"
                      + "\n\nexport QML_IMPORT_PATH=" + importPath
                      + "\n./yourapp -style=" + styleName
            }
        }

        ColumnLayout {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: childrenRect.height

            Label {
                text: figmaFileName
                visible: figmaFileName !== ""
            }

            Label {
                id: statusLabel
            }

            ProgressBar {
                id: progressBar
                Layout.fillWidth: true
            }
        }

        Button {
            id: generateButton
            Layout.alignment: Qt.AlignRight
            text: generating ? "Stop" : "Generate"
            enabled: !stopping
                && bridge.figmaUrlOrId !== ""
                && bridge.figmaToken !== ""
                && bridge.targetDirectory !== ""
            onClicked: {
                if (generating) {
                    stop()
                } else {
                    outputModel.clear()
                    filteredModel.clear()
                    figmaFileName = ""
                    bridge.generate()
                }
            }
            Component.onCompleted: forceActiveFocus()
        }

        ScrollView {
            id: outputScrollView
            visible: showLogCheckbox.checked
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: 200
            TableView {
                id: outputView
                clip: true
                reuseItems: false
                model: filteredModel
                delegate: Label {
                    text: msg
                    Component.onCompleted: {
                        if (implicitWidth > outputView.contentWidth)
                            outputView.contentWidth = implicitWidth
                    }
                }
            }
        }

        TextField {
            id: outputFilter
            placeholderText: "Filter (regexp)"
            Layout.preferredWidth: outputScrollView.width
            visible: outputScrollView.visible
            onAccepted: updateOutputFilter()
        }

    }
}

