/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import TestModel

ApplicationWindow {
    width: 800
    height: 600
    visible: true

    property alias treeView: treeView

    UICallback { id: callback }

    Item {
        anchors.fill: parent
        anchors.margins: 10

        Column {
            id: leftMenu
            width: childrenRect.width
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom

            CheckBox {
                id: useCustomDelegate
                text: "Use custom delegate"
            }

            CheckBox {
                id: useFileSystemModel
                text: "Use file system model"
            }

            Button {
                text: "Show QTreeView"
                onClicked: callback.showQTreeView(treeView.model)
            }

            Button {
                text: "Insert row"
                onClicked: {
                    let index = treeView.modelIndex(1, 0)
                    treeView.model.insertRows(index.row, 1, index.parent);
                }
            }

            Button {
                text: "Remove row"
                onClicked: {
                    let index = treeView.modelIndex(1, 0)
                    treeView.model.removeRows(index.row, 1, index.parent);
                }
            }
        }

        TreeView {
            id: treeView
            width: parent.width
            anchors.left: leftMenu.right
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.leftMargin: 100
            anchors.topMargin: 100
            clip: true

            model: useFileSystemModel.checked ? fileSystemModel : testModel
            delegate: useCustomDelegate.checked ? customDelegate : treeViewDelegate
        }
    }

    TestModel {
        id: testModel
    }

    Component {
        id: treeViewDelegate
        TreeViewDelegate {
            TapHandler {
                acceptedModifiers: Qt.ControlModifier
                onTapped: {
                    if (treeView.isExpanded(row))
                        treeView.collapseRecursively(row)
                    else
                        treeView.expandRecursively(row)
                }
            }
        }
    }

    Component {
        id: customDelegate
        Item {
            id: root

            implicitWidth: padding + label.x + label.implicitWidth + padding
            implicitHeight: label.implicitHeight * 1.5

            readonly property real indentation: 20
            readonly property real padding: 5

            // Assigned to by TreeView:
            required property TreeView treeView
            required property bool isTreeNode
            required property bool expanded
            required property int hasChildren
            required property int depth

            TapHandler {
                onTapped: treeView.toggleExpanded(row)
            }

            Text {
                id: indicator
                visible: root.isTreeNode && root.hasChildren
                x: padding + (root.depth * root.indentation)
                text: root.expanded ? "▼" : "▶"
            }

            Text {
                id: label
                x: padding + (root.isTreeNode ? (root.depth + 1) * root.indentation : 0)
                width: root.width - root.padding - x
                clip: true
                text: model.display
            }
        }
    }
}
