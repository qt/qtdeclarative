// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.qmlmodels

import Spreadsheets

ApplicationWindow {
    width: 960
    height: 720
    visible: true
    title: qsTr("Spreadsheets")

    header: HeaderToolBar {
        id: toolbar
        onHelpRequested: helpDialog.open()
        onPasteRequested: tableView.pasteFromClipboard()
        onCopyRequested: tableView.copyToClipboard()
        onCutRequested: tableView.cutToClipboard()
    }

    background: Rectangle {
        // to make contrast with the cells of the TableView,
        // HorizontalHeaderView and VerticalHeaderView
        color: Qt.styleHints.colorScheme === Qt.Light ? palette.dark : palette.light
    }

    GridLayout {
        id: gridlayout
        anchors.fill: parent
        anchors.margins: 4
        columns: 2
        rows: 2
        columnSpacing: 3
        rowSpacing: 3

        ColumnHeaderView {
            id: horizontalHeaderView
            Layout.row: 0
            Layout.column: 1
            Layout.fillWidth: true
            implicitHeight: 36
            clip: true
            syncView: tableView

            spreadSelectionModel: _spreadSelectionModel
            enableShowHideAction: tableView.hiddenColumnCount
            onHideRequested: (column) => tableView.hideColumn(column)
            onShowRequested: () => tableView.showHiddenColumns()
            onResetReorderingRequested: () => tableView.resetColumnReordering()
        }

        RowHeaderView {
            id: verticalHeaderView
            Layout.fillHeight: true
            implicitWidth: 50
            clip: true
            syncView: tableView

            spreadSelectionModel: _spreadSelectionModel
            enableShowHideAction: tableView.hiddenRowCount
            onHideRequested: (row) => tableView.hideRow(row)
            onShowRequested: () => tableView.showHiddenRows()
            onResetReorderingRequested: () => tableView.resetRowReordering()
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ScrollView {
                id: scrollView
                anchors.fill: parent

                contentItem: TableView {
                    id: tableView

                    property int hiddenColumnCount: 0
                    property int hiddenRowCount: 0

                    clip: true
                    columnSpacing: 2
                    rowSpacing: 2
                    boundsBehavior: Flickable.StopAtBounds
                    selectionBehavior: TableView.SelectCells
                    selectionMode: TableView.ExtendedSelection
                    selectionModel: _spreadSelectionModel
                    model: SpreadModel

                    function showHiddenColumns()
                    {
                        for (let column = 0; column < columns; ++column) {
                            if (explicitColumnWidth(column) === 0)
                                setColumnWidth(column, -1)
                        }
                        hiddenColumnCount = 0
                    }

                    function hideColumn(column)
                    {
                        if (column < 0)
                            return
                        setColumnWidth(column, 0)
                        ++hiddenColumnCount
                    }

                    function showHiddenRows()
                    {
                        for (let row = 0; row < rows; ++row) {
                            if (explicitRowHeight(row) === 0)
                                setRowHeight(row, -1)
                        }
                        hiddenRowCount = 0
                    }

                    function hideRow(row)
                    {
                        if (row < 0)
                            return
                        setRowHeight(row, 0)
                        ++hiddenRowCount
                    }

                    function copyToClipboard()
                    {
                        mimeDataProvider.reset()
                        if (_spreadSelectionModel.hasSelection) {
                            const source_index = _spreadSelectionModel.selectedIndexes[0]
                            mimeDataProvider.sourceCell = cellAtIndex(source_index)
                            mimeDataProvider.loadSelectedData()
                        } else {
                            const current_index = _spreadSelectionModel.currentIndex
                            const current_cell = cellAtIndex(current_index)
                            mimeDataProvider.sourceCell = current_cell
                            mimeDataProvider.loadDataFromModel(current_cell, current_index, model)
                        }
                    }

                    function cutToClipboard()
                    {
                        mimeDataProvider.reset()
                        if (_spreadSelectionModel.hasSelection) {
                            const source_index = _spreadSelectionModel.selectedIndexes[0]
                            mimeDataProvider.sourceCell = cellAtIndex(source_index)
                            mimeDataProvider.loadSelectedData()
                        } else {
                            const current_index = _spreadSelectionModel.currentIndex
                            const current_cell = cellAtIndex(current_index)
                            mimeDataProvider.sourceCell = current_cell
                            mimeDataProvider.loadDataFromModel(current_cell, current_index, model)
                        }
                        for (let i = 0; i < mimeDataProvider.size(); ++i) {
                            const cell_i = mimeDataProvider.cellAt(i)
                            model.clearItemData(index(cell_i.y, cell_i.x))
                        }
                    }

                    function pasteFromClipboard()
                    {
                        visibleCellsConnection.blockConnection(true)
                        const current_index = _spreadSelectionModel.currentIndex
                        const current_cell = cellAtIndex(current_index)

                        let target_cells = new Set()
                        if (mimeDataProvider.size() === 1) {
                            if (_spreadSelectionModel.hasSelection) {
                                for (let i in _spreadSelectionModel.selectedIndexes) {
                                    const selected_index = _spreadSelectionModel.selectedIndexes[i]
                                    mimeDataProvider.saveDataToModel(0, selected_index, model)
                                    target_cells.add(tableView.cellAtIndex(selected_index))
                                }
                            } else {
                                const old_cell = mimeDataProvider.cellAt(0)
                                let new_cell = Qt.point(old_cell.x, old_cell.y)
                                new_cell.x += current_cell.x - mimeDataProvider.sourceCell.x
                                new_cell.y += current_cell.y - mimeDataProvider.sourceCell.y
                                mimeDataProvider.saveDataToModel(0, index(new_cell.y, new_cell.x), model)
                                target_cells.add(new_cell)
                            }
                        } else if (mimeDataProvider.size() > 1) {
                            for (let i = 0; i < mimeDataProvider.size(); ++i) {
                                let cell_i = mimeDataProvider.cellAt(i)
                                cell_i.x += current_cell.x - mimeDataProvider.sourceCell.x
                                cell_i.y += current_cell.y - mimeDataProvider.sourceCell.y
                                mimeDataProvider.saveDataToModel(i, index(cell_i.y, cell_i.x), model)
                                target_cells.add(cell_i)
                            }
                        }
                        visibleCellsConnection.blockConnection(false)
                        visibleCellsConnection.updateViewArea()
                    }

                    function resetColumnReordering()
                    {
                        clearColumnReordering()
                        model.resetColumnMapping()
                    }

                    function resetRowReordering()
                    {
                        clearRowReordering()
                        model.resetRowMapping()
                    }

                    rowHeightProvider: function(row) {
                        const height = explicitRowHeight(row)
                        if (height === 0)
                            return 0
                        else if (height > 0)
                            return Math.max(height, 30)
                        return implicitRowWidth(row)
                    }

                    columnWidthProvider: function(column) {
                        const width = explicitColumnWidth(column)
                        if (width === 0)
                            return 0
                        else if (width > 0)
                            return Math.max(width, 30)
                        return implicitColumnWidth(column)
                    }

                    delegate: TableCell {
                        required property var model

                        implicitWidth: 90
                        implicitHeight: 36
                        text: model.display ?? ""
                        // We don't create data for empty cells to reduce
                        // the memory usage in case of huge model.
                        // If a cell does not have data and it's not highlighted neither
                        // the model.highlight is undefined which is replaced with false value.
                        highlight: model.highlight ?? false
                        edit: model.edit ?? ""

                        onCommit: text => model.edit = text
                    }

                    Keys.onPressed: function (event) {
                        if (event.matches(StandardKey.Copy)) {
                            copyToClipboard()
                        } else if (event.matches(StandardKey.Cut)) {
                            cutToClipboard()
                        } else if (event.matches(StandardKey.Paste)) {
                            pasteFromClipboard()
                        } else if (event.matches(StandardKey.Delete)) {
                            visibleCellsConnection.blockConnection()
                            if (_spreadSelectionModel.hasSelection)
                                model.clearItemData(_spreadSelectionModel.selectedIndexes)
                            else
                                model.clearItemData(_spreadSelectionModel.currentIndex)
                            visibleCellsConnection.blockConnection(false)
                            visibleCellsConnection.updateViewArea()
                        }
                    }

                    Connections {
                        id: visibleCellsConnection
                        target: SpreadModel

                        function onDataChanged(tl, br, roles)
                        {
                            updateViewArea()
                        }

                        // The model is updated, then the visible area needs to be updated as well.
                        // Maybe some cells need to get the display data again
                        // due to their data, if it's a formula.
                        function updateViewArea()
                        {
                            for (let row = tableView.topRow; row <= tableView.bottomRow; ++row) {
                                for (let column = tableView.leftColumn; column <= tableView.rightColumn; ++column) {
                                    SpreadModel.update(row, column)
                                }
                            }
                        }

                        // Blocks/unblocks the connection. This function is useful when
                        // some actions may update a large amount of data in the model,
                        // or may update a cell which affects other cells,
                        // for example clipboard actions and drag/drop actions.
                        // Block the connection, update the model, unblock the connection,
                        // and the call the updateViewArea() function to update the view.
                        function blockConnection(block=true)
                        {
                            visibleCellsConnection.enabled = !block
                        }
                    }

                    MouseArea {
                        id: dragArea

                        property point dragCell: Qt.point(-1, -1)
                        property bool hadSelection: false

                        anchors.fill: parent
                        drag.axis: Drag.XandYAxis
                        drag.target: dropArea
                        acceptedButtons: Qt.LeftButton
                        cursorShape: drag.active ? Qt.ClosedHandCursor : Qt.ArrowCursor

                        onPressed: function(mouse) {
                            mouse.accepted = false
                            // only when Alt modifier is pressed
                            if (mouse.modifiers !== Qt.AltModifier)
                                return
                            // check cell under press position
                            const position = Qt.point(mouse.x, mouse.y)
                            const cell = tableView.cellAtPosition(position, true)
                            if (cell.x < 0 || cell.y < 0)
                                return
                            // check selected indexes
                            const index = tableView.index(cell.y, cell.x)
                            hadSelection = _spreadSelectionModel.hasSelection
                            if (!hadSelection)
                                _spreadSelectionModel.select(index, ItemSelectionModel.Select)
                            if (!_spreadSelectionModel.isSelected(index))
                                return
                            // store selected data
                            mimeDataProvider.reset()
                            mimeDataProvider.loadSelectedData()
                            // accept dragging
                            if (mimeDataProvider.size() > 0) {
                                mouse.accepted = true
                                dragCell = cell
                            }

                            dropArea.startDragging()
                        }

                        onReleased: {
                            dropArea.stopDragging()
                            // reset selection, if dragging caused the selection
                            if (!hadSelection)
                                _spreadSelectionModel.clearSelection()
                            hadSelection = false
                            dragCell = Qt.point(-1, -1)
                        }
                    }

                    DropArea {
                        id: dropArea

                        property point dropCell: Qt.point(-1, -1)

                        anchors.fill: parent
                        Drag.active: dragArea.drag.active

                        function startDragging()
                        {
                            // block updating visible area
                            visibleCellsConnection.blockConnection()
                        }

                        function stopDragging()
                        {
                            Drag.drop()
                            // unblock update visible area
                            visibleCellsConnection.blockConnection(false)
                            visibleCellsConnection.updateViewArea()  // now update visible area
                        }

                        onDropped: {
                            const position = Qt.point(dragArea.mouseX, dragArea.mouseY)
                            dropCell = tableView.cellAtPosition(position, true)
                            if (dropCell.x < 0 || dropCell.y < 0)
                                return
                            if (dragArea.dragCell === dropCell)
                                return

                            tableView.model.clearItemData(_spreadSelectionModel.selectedIndexes)
                            for (let i = 0; i < mimeDataProvider.size(); ++i) {
                                let cell = mimeDataProvider.cellAt(i)
                                cell.x += dropCell.x - dragArea.dragCell.x
                                cell.y += dropCell.y - dragArea.dragCell.y
                                const index = tableView.index(cell.y, cell.x)
                                mimeDataProvider.saveDataToModel(i, index, tableView.model)
                            }
                            mimeDataProvider.reset()
                            _spreadSelectionModel.clearSelection()

                            const drop_index = tableView.index(dropCell.y, dropCell.x)
                            _spreadSelectionModel.setCurrentIndex(drop_index, ItemSelectionModel.Current)

                            tableView.model.clearHighlight()
                        }

                        onPositionChanged: {
                            const position = Qt.point(dragArea.mouseX, dragArea.mouseY)
                            // cell is the cell that currently mouse is over it
                            const cell = tableView.cellAtPosition(position, true)
                            // dropCell is the cell that it was under the mouse's last position
                            // if the last and current cells are the same, then there is no need
                            // to update highlight, as nothing is changed since last time.
                            if (cell === dropCell)
                                return
                            // if something is changed, it means that if the current cell is changed,
                            // then clear highlighted cells and update the dropCell.
                            tableView.model.clearHighlight()
                            dropCell = cell
                            // if the current cell was invalid (mouse is out side of the TableView)
                            // then no need to update highlight
                            if (cell.x < 0 || cell.y < 0)
                                return
                            // if dragged cell is the same as the (possibly) dropCell
                            // then no need to highlight any cells
                            if (dragArea.dragCell === dropCell)
                                return
                            // if the dropCell is not the same as the dragging cell and also
                            // is not the same as the cell at the mouse's last position
                            // then highlights the target cells
                            for (let i in _spreadSelectionModel.selectedIndexes) {
                                const old_index = _spreadSelectionModel.selectedIndexes[i]
                                let cell = tableView.cellAtIndex(old_index)
                                cell.x += dropCell.x - dragArea.dragCell.x
                                cell.y += dropCell.y - dragArea.dragCell.y
                                const new_index = tableView.index(cell.y, cell.x)
                                tableView.model.setHighlight(new_index, true)
                            }
                        }
                    }
                }
            }
        }
    }

    SelectionRectangle {
        id: selectionRectangle
        target: tableView
        selectionMode: SelectionRectangle.Auto

        topLeftHandle: Rectangle {
            width: 12
            height: 12
            radius: 6
            color: palette.highlight.lighter(1.4)
            border.width: 2
            border.color: palette.base
            visible: SelectionRectangle.control.active
        }

        bottomRightHandle: Rectangle {
            width: 12
            height: 12
            radius: 6
            color: palette.highlight.lighter(1.4)
            border.width: 2
            border.color: palette.base
            visible: SelectionRectangle.control.active
        }
    }

    SpreadSelectionModel {
        id: _spreadSelectionModel
        behavior: SpreadSelectionModel.SelectCells
    }

    SpreadMimeDataProvider {
        id: mimeDataProvider

        property point sourceCell: Qt.point(-1, -1)

        function loadSelectedData()
        {
            for (let i in _spreadSelectionModel.selectedIndexes) {
                const index = _spreadSelectionModel.selectedIndexes[i]
                const cell = tableView.cellAtIndex(index)
                loadDataFromModel(cell, index, tableView.model)
            }
        }

        function resetProvider()
        {
            sourceCell = Qt.point(-1, -1)
            reset()
        }
    }

    HelpDialog {
        id: helpDialog
        anchors.centerIn: parent
    }
}
