// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

import Spreadsheets

HorizontalHeaderView {
    id: root

    property alias enableShowHideAction: showHideMenuItem.enabled
    required property SpreadSelectionModel spreadSelectionModel

    signal resetReorderingRequested()
    signal hideRequested(int column)
    signal showRequested()

    selectionBehavior: HorizontalHeaderView.SelectionDisabled
    movableColumns: true
    onColumnMoved: (index, old_column, new_column) => model.mapColumn(index, new_column)

    selectionModel: HeaderSelectionModel {
        id: headerSelectionModel
        selectionModel: spreadSelectionModel
        orientation: Qt.Horizontal
    }

    delegate: Rectangle {
        id: headerDelegate

        required property var index
        required property bool selected
        required property bool containsDrag
        readonly property real cellPadding: 8
        readonly property bool containsMenu: columnMenu.column === column

        implicitWidth: title.implicitWidth + (cellPadding * 2)
        implicitHeight: Math.max(root.height, title.implicitHeight + (cellPadding * 2))
        border {
            width: containsDrag || containsMenu ? 1 : 0
            color: palette.highlight
        }
        color: selected ? palette.highlight : palette.button

        gradient: Gradient {
            GradientStop {
                position: 0
                color: Qt.styleHints.colorScheme === Qt.Light ? headerDelegate.color
                                                              : Qt.lighter(headerDelegate.color, 1.3)
            }
            GradientStop {
                position: 1
                color: Qt.styleHints.colorScheme === Qt.Light ? Qt.darker(headerDelegate.color, 1.3)
                                                              : headerDelegate.color
            }
        }

        function rightClicked() {
            columnMenu.column = index
            const menu_pos = mapToItem(root, -anchors.margins, height + anchors.margins)
            columnMenu.popup(menu_pos)
        }

        Label {
            id: title
            anchors.centerIn: parent
            text: model.columnName
        }

        HeaderViewTapHandler {
            anchors.fill: parent
            onToggleRequested: {
                spreadSelectionModel.toggleColumn(index)
                headerSelectionModel.setCurrent()
            }
            onSelectRequested: {
                spreadSelectionModel.selectColumn(index)
                headerSelectionModel.setCurrent()
            }
            onContextMenuRequested: headerDelegate.rightClicked()
        }
    }

    Menu {
        id: columnMenu

        property int column: -1

        onOpened: {
            headerSelectionModel.setCurrent(column)
        }

        onClosed: {
            headerSelectionModel.setCurrent()
            column = -1
        }

        MenuItem {
            text: qsTr("Insert 1 column left")
            icon {
                source: "icons/insert_column_left.svg"
                color: palette.highlightedText
            }

            onClicked: {
                if (columnMenu.column < 0)
                    return
                SpreadModel.insertColumn(columnMenu.column)
            }
        }

        MenuItem {
            text: qsTr("Insert 1 column right")
            icon {
                source: "icons/insert_column_right.svg"
                color: palette.highlightedText
            }

            onClicked: {
                if (columnMenu.column < 0)
                    return
                SpreadModel.insertColumn(columnMenu.column + 1)
            }
        }

        MenuItem {
            text: selectionModel.hasSelection ? qsTr("Remove selected columns")
                                              : qsTr("Remove column")
            icon {
                source: "icons/remove_column.svg"
                color: palette.text
            }

            onClicked: {
                if (selectionModel.hasSelection)
                    SpreadModel.removeColumns(spreadSelectionModel.selectedColumns())
                else if (columnMenu.column >= 0)
                    SpreadModel.removeColumn(columnMenu.column)
            }
        }

        MenuItem {
            text: selectionModel.hasSelection ? qsTr("Hide selected columns")
                                              : qsTr("Hide column")
            icon {
                source: "icons/hide.svg"
                color: palette.text
            }

            onClicked: {
                if (selectionModel.hasSelection) {
                    let columns = spreadSelectionModel.selectedColumns()
                    columns.sort(function(lhs, rhs){ return rhs.column - lhs.column })
                    for (let i in columns)
                        root.hideRequested(columns[i].column)
                    spreadSelectionModel.clearSelection()
                } else {
                    root.hideRequested(columnMenu.column)
                }
            }
        }

        MenuItem {
            id: showHideMenuItem
            text: qsTr("Show hidden column(s)")
            icon {
                source: "icons/show.svg"
                color: palette.text
            }

            onClicked: {
                root.showRequested()
                spreadSelectionModel.clearSelection()
            }
        }

        MenuItem {
            text: qsTr("Reset column reordering")
            icon {
                source: "icons/reset_reordering.svg"
                color: palette.text
            }

            onClicked: root.resetReorderingRequested()
        }
    }
}
