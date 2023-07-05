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

    Connections {
        target: bridge

        function onDebug(msg) {
            verboseOutput(msg)
        }

        function onWarning(msg) {
            if (stopping)
                return
            output("Warning: " + msg)
        }

        function onError(msg) {
            if (stopping)
                return
            output("Error: " + msg)
        }

        function onStarted() {
            generating = true
        }

        function onFinished() {
            progressBar.value = 0
            progressBar.indeterminate = false
            statusLabel.text = stopping ? "Stopped!" : "Finished!"
            verboseOutput(stopping ? "Stopped!" : "Finished!")
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
            verboseOutput(label)
        }

        function onProgress() {
            progressBar.value++
        }
    }

    function verboseOutput(msg)
    {
        if (!bridge.verbose || stopping)
            return
        output(msg)
    }

    function output(msg)
    {
        outputModel.append({"msg": msg})
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
                Button {
                    flat: true
                    icon.source: "qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/folder-icon-square.png"
                    onClicked: stylePathDialog.open()
                }
                FolderDialog {
                    id: stylePathDialog
                    onAccepted: stylePathInput.text = selectedFolder
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
            }

            Label {
                text: "Figma Token"
            }

            TextField {
                placeholderText: "Token"
                Layout.fillWidth: true
                text: bridge.figmaToken
                onTextChanged: bridge.figmaToken = text
            }

            Label {
                text: "Overwrite QML"
            }

            CheckBox {
                checked: bridge.overwriteQml
                onCheckedChanged: bridge.overwriteQml = checked
            }
        }

        ColumnLayout {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: childrenRect.height
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
                    bridge.generate()
                }
            }
            Component.onCompleted: forceActiveFocus()
        }

        ScrollView {
            id: outputScrollView
            visible: outputModel.count > 0
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: 200
            TableView {
                id: outputView
                clip: true
                reuseItems: false
                model: outputModel
                delegate: Text {
                    text: msg
                    Component.onCompleted: {
                        if (implicitWidth > outputView.contentWidth)
                            outputView.contentWidth = implicitWidth
                    }
                }
            }
        }

    }
}

