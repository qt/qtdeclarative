// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import FileSystemModule

// This is the file system view which gets populated by the C++ model.
Rectangle {
    id: root

    signal fileClicked(string filePath)

    TreeView {
        id: fileSystemTreeView
        anchors.fill: parent
        model: FileSystemModel
        boundsBehavior: Flickable.StopAtBounds
        boundsMovement: Flickable.StopAtBounds
        clip: true

        property int lastIndex: -1

        Component.onCompleted: fileSystemTreeView.toggleExpanded(0)

        // The delegate represents a single entry in the filesystem.
        delegate: TreeViewDelegate {
            id: treeDelegate
            indentation: 8
            implicitWidth: fileSystemTreeView.width > 0 ? fileSystemTreeView.width : 250
            implicitHeight: 25

            required property int index
            required property url filePath

            indicator: null

            contentItem: Item {
                anchors.fill: parent

                Icon {
                    id: directoryIcon
                    x: leftMargin + (depth * indentation)
                    anchors.verticalCenter: parent.verticalCenter
                    path: treeDelegate.hasChildren
                        ? (treeDelegate.expanded ? "../icons/folder_open.svg" : "../icons/folder_closed.svg")
                        : "../icons/generic_file.svg"
                    iconColor: (treeDelegate.expanded && treeDelegate.hasChildren) ? Colors.color2 : Colors.folder
                }
                Text {
                    anchors.left: directoryIcon.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width
                    text: model.fileName
                    color: Colors.text
                }
            }

            background: Rectangle {
                color: treeDelegate.index === fileSystemTreeView.lastIndex
                    ? Colors.selection
                    : (hoverHandler.hovered ? Colors.active : "transparent")
            }

            TapHandler {
                onSingleTapped: {
                    fileSystemTreeView.toggleExpanded(row)
                    fileSystemTreeView.lastIndex = index
                    // If this model item doesn't have children, it means it's representing a file.
                    if (!treeDelegate.hasChildren)
                        root.fileClicked(filePath)
                }
            }
            HoverHandler {
                id: hoverHandler
            }
        }

        // Provide our own custom ScrollIndicator for the TreeView.
        ScrollIndicator.vertical: ScrollIndicator {
            active: true
            implicitWidth: 15

            contentItem: Rectangle {
                implicitWidth: 6
                implicitHeight: 6
                color: Colors.color1
                opacity: fileSystemTreeView.movingVertically ? 0.5 : 0.0

                Behavior on opacity {
                    OpacityAnimator {
                        duration: 500
                    }
                }
            }
        }
    }
}
