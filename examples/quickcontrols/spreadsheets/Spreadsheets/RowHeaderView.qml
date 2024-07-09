// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

import Spreadsheets

VerticalHeaderView {
    id: root

    property alias enableShowHideAction: showHideMenuItem.enabled
    required property SpreadSelectionModel spreadSelectionModel

    signal resetReorderingRequested()
    signal hideRequested(int row)
    signal showRequested()

    selectionBehavior: VerticalHeaderView.SelectionDisabled
    movableRows: true
    onRowMoved: (index, old_row, new_row) => model.mapRow(index, new_row)

    selectionModel: HeaderSelectionModel {
        id: headerSelectionModel
        selectionModel: spreadSelectionModel
        orientation: Qt.Vertical
    }

    delegate: Rectangle {
        id: headerDelegate

        required property var index
        required property bool selected
        required property bool current
        required property bool containsDrag
        readonly property real cellPadding: 8
        readonly property bool containsMenu: rowMenu.row === row

        implicitHeight: title.implicitHeight + (cellPadding * 2)
        implicitWidth: Math.max(root.width, title.implicitWidth + (cellPadding * 2))

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
            rowMenu.row = index
            const menu_pos = mapToItem(root, width + anchors.margins, -anchors.margins)
            rowMenu.popup(menu_pos)
        }

        Label {
            id: title
            anchors.centerIn: parent
            text: model.rowName
        }

        HeaderViewTapHandler {
            anchors.fill: parent
            onToggleRequested: {
                spreadSelectionModel.toggleRow(index)
                headerSelectionModel.setCurrent()
            }
            onSelectRequested: {
                spreadSelectionModel.selectRow(index)
                headerSelectionModel.setCurrent()
            }
            onContextMenuRequested: headerDelegate.rightClicked()
        }
    }

    Menu {
        id: rowMenu

        property int row: -1

        onOpened: {
            headerSelectionModel.setCurrent(row)
        }

        onClosed: {
            headerSelectionModel.setCurrent()
            row = -1
        }

        MenuItem {
            text: qsTr("Insert 1 row above")
            icon {
                source: "icons/insert_row_above.svg"
                color: palette.highlightedText
            }

            onClicked: {
                if (rowMenu.row < 0)
                    return
                SpreadModel.insertRow(rowMenu.row)
            }
        }

        MenuItem {
            text: qsTr("Insert 1 row bellow")
            icon {
                source: "icons/insert_row_below.svg"
                color: palette.text
            }

            onClicked: {
                if (rowMenu.row < 0)
                    return
                SpreadModel.insertRow(rowMenu.row + 1)
            }
        }

        MenuItem {
            text: selectionModel.hasSelection ? qsTr("Remove selected rows")
                                              : qsTr("Remove row")
            icon {
                source: "icons/remove_row.svg"
                color: palette.text
            }

            onClicked: {
                if (selectionModel.hasSelection)
                    SpreadModel.removeRows(selectionModel.selectedRows())
                else if (rowMenu.row >= 0)
                    SpreadModel.removeRow(rowMenu.row)
            }
        }

        MenuItem {
            text: selectionModel.hasSelection ? qsTr("Hide selected rows")
                                              : qsTr("Hide row")
            icon {
                source: "icons/hide.svg"
                color: palette.text
            }

            onClicked: {
                if (selectionModel.hasSelection) {
                    let rows = selectionModel.selectedRows()
                    rows.sort(function(lhs, rhs){ return rhs.row - lhs.row })
                    for (let i in rows)
                        root.hideRequested(rows[i].row)
                    spreadSelectionModel.clearSelection()
                } else {
                    root.hideRequested(rowMenu.row)
                }
            }
        }

        MenuItem {
            id: showHideMenuItem
            text: qsTr("Show hidden row(s)")
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
            text: qsTr("Reset row reordering")
            icon {
                source: "icons/reset_reordering.svg"
                color: palette.text
            }

            onClicked: root.resetReorderingRequested()
        }
    }
}
