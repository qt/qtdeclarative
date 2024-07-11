// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl

ApplicationWindow {
    width: 800
    height: 600
    visible: true
    title: "Table of Contents"

    TreeView {
        id: treeView
        anchors.fill: parent
        anchors.margins: 10
        clip: true

        selectionModel: ItemSelectionModel {}

        model: TreeModel { }

        delegate: TreeViewDelegate {
            id: viewDelegate
            readonly property real _padding: 5
            readonly property real szHeight: contentItem.implicitHeight * 2.5

            implicitWidth: _padding + contentItem.x + contentItem.implicitWidth + _padding
            implicitHeight: szHeight

            background: Rectangle { // Background rectangle enabled to show the alternative row colors
                id: background
                anchors.fill: parent
                color: {
                    if (viewDelegate.model.row === viewDelegate.treeView.currentRow) {
                        return Qt.lighter(palette.highlight, 1.2)
                    } else {
                        if (viewDelegate.treeView.alternatingRows && viewDelegate.model.row % 2 !== 0) {
                            return (Application.styleHints.colorScheme === Qt.Light) ?
                                     Qt.darker(palette.alternateBase, 1.25) :
                                     Qt.lighter(palette.alternateBase, 2.)
                        } else {
                           return palette.base
                        }
                    }
                }
                Rectangle { // The selection indicator shown on the left side of the highlighted row
                    width: viewDelegate._padding
                    height: parent.height
                    visible: !viewDelegate.model.column
                    color: {
                        if (viewDelegate.model.row === viewDelegate.treeView.currentRow) {
                            return (Application.styleHints.colorScheme === Qt.Light) ?
                                     Qt.darker(palette.highlight, 1.25) :
                                     Qt.lighter(palette.highlight, 2.)
                        } else {
                            return "transparent"
                        }
                    }
                }
            }

            indicator: Item {
                x: viewDelegate._padding + viewDelegate.depth * viewDelegate.indentation
                implicitWidth: viewDelegate.szHeight
                implicitHeight: viewDelegate.szHeight
                visible: viewDelegate.isTreeNode && viewDelegate.hasChildren
                rotation: viewDelegate.expanded ? 90 : 0
                TapHandler {
                    onSingleTapped: {
                        let index = viewDelegate.treeView.index(viewDelegate.model.row, viewDelegate.model.column)
                        viewDelegate.treeView.selectionModel.setCurrentIndex(index, ItemSelectionModel.NoUpdate)
                        viewDelegate.treeView.toggleExpanded(viewDelegate.model.row)
                    }
                }
                ColorImage {
                    width: parent.width / 3
                    height: parent.height / 3
                    anchors.centerIn: parent
                    source: "qrc:/arrow_icon.png"
                    color: palette.buttonText
                }
            }

            contentItem: Label {
                x: viewDelegate._padding + (viewDelegate.depth + 1 * viewDelegate.indentation)
                width: parent.width - viewDelegate._padding - x
                text: viewDelegate.model.display
                elide: Text.ElideRight
            }
        }
    }
}
