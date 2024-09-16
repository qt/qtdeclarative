// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Qt.labs.folderlistmodel

ApplicationWindow {
    id: window
    title: "Qt Quick Controls 2 - Screenshots"
    visible: true
    width: Math.max(600, loader.implicitWidth)
    height: Math.max(600, loader.implicitHeight + header.implicitHeight + footer.implicitHeight)

    property string currentFilePath
    property url lastSaveUrl

    Shortcut {
        sequence: "Ctrl+Q"
        onActivated: Qt.quit()
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent

            ToolButton {
                text: "Choose Snippet"
                focusPolicy: Qt.NoFocus
                onClicked: snippetDrawer.open()
            }
        }
    }

    Drawer {
        id: snippetDrawer
        width: window.width / 2
        height: window.height

        ListView {
            id: snippetsListView
            anchors.fill: parent
            model: FolderListModel {
                folder: snippetsDir
                nameFilters: ["*.qml"]
                showDirs: false
            }
            delegate: ItemDelegate {
                width: snippetsListView.width
                text: fileName
                focusPolicy: Qt.NoFocus

                readonly property string baseName: fileBaseName

                contentItem: Label {
                    text: parent.text
                    elide: Text.ElideLeft
                }
                onClicked: {
                    snippetsListView.currentIndex = index;
                    loader.source = "file:///" + filePath;
                    currentFilePath = filePath;
                    snippetDrawer.close();
                }
            }
        }
    }

    Loader {
        id: loader
        anchors.centerIn: parent
    }

    ToolTip {
        id: saveResultToolTip
        x: window.contentItem.width / 2 - width / 2
        y: window.contentItem.height - height - 20
        timeout: 3000
    }

    footer: ToolBar {
        RowLayout {
            anchors.fill: parent

            ToolButton {
                text: "Open Output Folder"
                focusPolicy: Qt.NoFocus
                onClicked: Qt.openUrlExternally(screenshotsDir)
            }

            ToolButton {
                text: "Open Last Screenshot"
                focusPolicy: Qt.NoFocus
                enabled: lastSaveUrl.toString().length > 0
                onClicked: Qt.openUrlExternally(lastSaveUrl)
            }

            Item {
                Layout.fillWidth: true
            }

            ToolButton {
                text: "Take Screenshot"
                focusPolicy: Qt.NoFocus
                enabled: loader.status === Loader.Ready
                onClicked: {
                    if (!loader.item)
                        return;

                    var grabSuccessful = loader.grabToImage(function(result) {
                        var savePath = screenshotsDirStr + "/" + snippetsListView.currentItem.baseName + ".png";
                        if (result.saveToFile(savePath)) {
                            saveResultToolTip.text = "Successfully saved screenshot to output folder";
                            lastSaveUrl = screenshotsDir + "/" + snippetsListView.currentItem.baseName + ".png";
                        } else {
                            saveResultToolTip.text = "Failed to save screenshot";
                        }
                    })
                    if (!grabSuccessful)
                        saveResultToolTip.text = "Failed to grab image";
                    saveResultToolTip.open();
                }
            }
        }
    }
}
