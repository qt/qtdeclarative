// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import FileSystemModule

ApplicationWindow {
    id: root
    width: 1100
    height: 600
    visible: true
    flags: Qt.Window | Qt.FramelessWindowHint
    title: qsTr("Qt Quick Controls 2 - File System Explorer")

    property string currentFilePath: ""
    property bool expandPath: false

    menuBar: MyMenuBar {
        rootWindow: root

        infoText: currentFilePath
            ? (expandPath ? currentFilePath
            : currentFilePath.substring(currentFilePath.lastIndexOf("/") + 1, currentFilePath.length))
            : "File System Explorer"

        MyMenu {
            title: qsTr("File")

            Action {
                text: qsTr("Increase Font")
                shortcut: "Ctrl++"
                onTriggered: textArea.font.pixelSize += 1
            }
            Action {
                text: qsTr("Decrease Font")
                shortcut: "Ctrl+-"
                onTriggered: textArea.font.pixelSize -= 1
            }
            Action {
                text: expandPath ? qsTr("Toggle Short Path") : qsTr("Toggle Expand Path")
                enabled: currentFilePath
                onTriggered: expandPath = !expandPath
            }
            Action {
                text: qsTr("Exit")
                onTriggered: Qt.exit(0)
            }
        }

        MyMenu {
            title: qsTr("Edit")

            Action {
                text: qsTr("Cut")
                shortcut: StandardKey.Cut
                enabled: textArea.selectedText.length > 0
                onTriggered: textArea.cut()
            }
            Action {
                text: qsTr("Copy")
                shortcut: StandardKey.Copy
                enabled: textArea.selectedText.length > 0
                onTriggered: textArea.copy()
            }
            Action {
                text: qsTr("Paste")
                shortcut: StandardKey.Paste
                enabled: textArea.canPaste
                onTriggered: textArea.paste()
            }
            Action {
                text: qsTr("Select All")
                shortcut: StandardKey.SelectAll
                enabled: textArea.length > 0
                onTriggered: textArea.selectAll()
            }
            Action {
                text: qsTr("Undo")
                shortcut: StandardKey.Undo
                enabled: textArea.canUndo
                onTriggered: textArea.undo()
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Colors.background

        RowLayout {
            anchors.fill: parent
            spacing: 0

            // Stores the buttons that navigate the application.
            Sidebar {
                id: sidebar
                rootWindow: root

                Layout.preferredWidth: 60
                Layout.fillHeight: true
            }

            // Allows resizing parts of the UI.
            SplitView {
                Layout.fillWidth: true
                Layout.fillHeight: true

                handle: Rectangle {
                    implicitWidth: 10
                    color: SplitHandle.pressed ? Colors.color2 : Colors.background
                    border.color: Colors.color2
                    opacity: SplitHandle.hovered || SplitHandle.pressed ? 1.0 : 0.0

                    Behavior on opacity {
                        OpacityAnimator {
                            duration: 900
                        }
                    }
                }

                // We use an inline component to make a reusable TextArea component.
                // This is convenient when the component is only used in one file.
                component MyTextArea: TextArea {
                    antialiasing: true
                    color: Colors.textFile
                    selectedTextColor: Colors.textFile
                    selectionColor: Colors.selection
                    renderType: Text.QtRendering
                    textFormat: TextEdit.PlainText

                    background: null
                }

                Rectangle {
                    color: Colors.surface1

                    SplitView.preferredWidth: 250
                    SplitView.fillHeight: true

                    StackLayout {
                        currentIndex: sidebar.currentTabIndex

                        anchors.fill: parent

                        // Shows the help text.
                        MyTextArea {
                            readOnly: true
                            text: qsTr("This example shows how to use and visualize the file system.\n\n"
                                + "Customized Qt Quick Components have been used to achieve this look.\n\n"
                                + "You can edit the files but they won't be changed on the file system.\n\n"
                                + "Click on the folder icon to the left to get started.")
                            wrapMode: TextArea.Wrap
                        }

                        // Shows the files on the file system.
                        FileSystemView {
                            id: fileSystemView
                            color: Colors.surface1

                            onFileClicked: (path) => root.currentFilePath = path
                        }
                    }
                }

                // The ScrollView that contains the TextArea which shows the file's content.
                ScrollView {
                    leftPadding: 20
                    topPadding: 20
                    bottomPadding: 20
                    clip: true

                    SplitView.fillWidth: true
                    SplitView.fillHeight: true

                    property alias textArea: textArea

                    MyTextArea {
                        id: textArea
                        text: FileSystemModel.readFile(root.currentFilePath)
                    }
                }
            }
        }
        ResizeButton {}
    }
}
