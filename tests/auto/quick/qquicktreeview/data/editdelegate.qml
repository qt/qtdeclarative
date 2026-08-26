// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window

Item {
    width: 800
    height: 600

    property alias treeView: treeView
    property alias textInput: textInput

    TextInput {
        id: textInput
        width: 100
        height: 10
    }

    TreeView {
        id: treeView
        anchors.fill: parent
        clip: true

        property Item editItem: null
        property var editIndex

        selectionModel: ItemSelectionModel {}

        delegate: Rectangle {
            id: root
            implicitWidth: 100
            implicitHeight: 50
            clip: true
            color: current || selected ? "lightgreen" : "white"

            property alias text: label.text

            readonly property real indent: 20
            readonly property real padding: 5

            // Assigned to by TreeView:
            required property TreeView treeView
            required property bool isTreeNode
            required property bool expanded
            required property int hasChildren
            required property int depth
            required property bool current
            required property bool selected
            required property bool editing

            Text {
                id: indicator
                visible: root.isTreeNode && root.hasChildren
                x: padding + (root.depth * root.indent)
                text: root.expanded ? "▼" : "▶"
            }

            Text {
                id: label
                x: padding + (root.isTreeNode ? (root.depth + 1) * root.indent : 0)
                width: root.width - root.padding - x
                clip: true
                text: model.display
            }

            TableView.editDelegate: TextInput {
                id: editRoot
                anchors.fill: parent
                text: display
                horizontalAlignment: TextInput.AlignHCenter
                verticalAlignment: TextInput.AlignVCenter
                activeFocusOnTab: true

                required property bool editing

                Component.onCompleted: {
                    treeView.editItem = editRoot
                    treeView.editIndex = treeView.index(row, column)
                    selectAll()
                }

                Component.onDestruction: {
                    treeView.editItem = null
                    treeView.editIndex = treeView.index(-1, -1)
                }
            }
        }
    }

}
