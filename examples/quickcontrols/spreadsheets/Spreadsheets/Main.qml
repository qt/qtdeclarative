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
        panEnabled: false
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

        HorizontalHeaderView {
            id: horizontalHeaderView
            Layout.row: 0
            Layout.column: 1
            Layout.fillWidth: true
            implicitHeight: 36
            clip: true
            interactive: toolbar.panEnabled
            syncView: tableView

            selectionModel: HeaderSelectionModel {
                id: horizontalHeaderSelectionModel
                selectionModel: selectionModel
                orientation: Qt.Horizontal
            }

            movableColumns: true
            onColumnMoved: (index, old_column, new_column) => model.mapColumn(index, new_column)

            delegate: Rectangle {
                id: horizontalHeaderDelegate

                required property var index
                required property bool selected
                required property bool current
                required property bool containsDrag
                readonly property real cellPadding: 8

                implicitWidth: horizontalTitle.implicitWidth + (cellPadding * 2)
                implicitHeight: Math.max(horizontalHeaderView.height,
                                         horizontalTitle.implicitHeight + (cellPadding * 2))
                border {
                    width: containsDrag || current ? 1 : 0
                    color: palette.highlight
                }
                color: selected ? palette.highlight : palette.button

                gradient: Gradient {
                    GradientStop {
                        position: 0
                        color: Qt.styleHints.colorScheme === Qt.Light ? horizontalHeaderDelegate.color
                                                                      : Qt.lighter(horizontalHeaderDelegate.color, 1.3)
                    }
                    GradientStop {
                        position: 1
                        color: Qt.styleHints.colorScheme === Qt.Light ? Qt.darker(horizontalHeaderDelegate.color, 1.3)
                                                                      : horizontalHeaderDelegate.color
                    }
                }

                Label {
                    id: horizontalTitle
                    anchors.centerIn: parent
                    text: model.columnName
                }

                MouseArea {
                    anchors.fill: parent
                    anchors.margins: horizontalHeaderDelegate.cellPadding / 2
                    acceptedButtons: Qt.LeftButton | Qt.RightButton

                    onPressed: function(event) {
                        if (event.modifiers === Qt.AltModifier) {
                            event.accepted = false
                            return
                        }
                    }

                    onClicked: function(event) {
                        switch (event.button) {
                        case Qt.LeftButton:
                            if (event.modifiers & Qt.ControlModifier)
                                selectionModel.toggleColumn(index)
                            else
                                selectionModel.selectColumn(index)
                            break
                        case Qt.RightButton:
                            columnMenu.column = index
                            const menu_pos = mapToItem(horizontalHeaderView, -anchors.margins, height + anchors.margins)
                            columnMenu.popup(menu_pos)
                            break
                        }
                    }
                }
            }
            Menu {
                id: columnMenu

                property int column: -1

                onOpened: {
                    horizontalHeaderSelectionModel.setCurrent(column)
                }

                onClosed: {
                    horizontalHeaderSelectionModel.setCurrent()
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
                            SpreadModel.removeColumns(selectionModel.selectedColumns())
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
                            let columns = selectionModel.selectedColumns()
                            columns.sort(function(lhs, rhs){ return rhs.column - lhs.column })
                            for (let i in columns)
                                tableView.hideColumn(columns[i].column)
                            selectionModel.clearSelection()
                        } else {
                            tableView.hideColumn(columnMenu.column)
                        }
                    }
                }

                MenuItem {
                    text: qsTr("Show hidden column(s)")
                    icon {
                        source: "icons/show.svg"
                        color: palette.text
                    }

                    enabled: tableView.hiddenColumnCount

                    onClicked: {
                        tableView.showHiddenColumns()
                        selectionModel.clearSelection()
                    }
                }

                MenuItem {
                    text: qsTr("Reset column reordering")
                    icon {
                        source: "icons/reset_reordering.svg"
                        color: palette.text
                    }

                    onClicked: tableView.resetColumnReordering()
                }
            }
        }

        VerticalHeaderView {
            id: verticalHeaderView

            Layout.fillHeight: true
            implicitWidth: 50
            clip: true
            syncView: tableView
            interactive: toolbar.panEnabled
            movableRows: true

            selectionModel: HeaderSelectionModel {
                id: verticalHeaderSelectionModel
                selectionModel: selectionModel
                orientation: Qt.Vertical
            }

            onRowMoved: (index, old_row, new_row) => model.mapRow(index, new_row)

            delegate: Rectangle {
                id: verticalHeaderDelegate

                required property var index
                required property bool selected
                required property bool current
                required property bool containsDrag
                readonly property real cellPadding: 8

                implicitHeight: verticalTitle.implicitHeight + (cellPadding * 2)
                implicitWidth: Math.max(verticalHeaderView.width,
                                        verticalTitle.implicitWidth + (cellPadding * 2))

                border {
                    width: containsDrag || current ? 1 : 0
                    color: palette.highlight
                }

                color: selected ? palette.highlight : palette.button

                gradient: Gradient {
                    GradientStop {
                        position: 0
                        color: Qt.styleHints.colorScheme === Qt.Light ? verticalHeaderDelegate.color
                                                                      : Qt.lighter(verticalHeaderDelegate.color, 1.3)
                    }
                    GradientStop {
                        position: 1
                        color: Qt.styleHints.colorScheme === Qt.Light ? Qt.darker(verticalHeaderDelegate.color, 1.3)
                                                                      : verticalHeaderDelegate.color
                    }
                }

                Label {
                    id: verticalTitle
                    anchors.centerIn: parent
                    text: model.rowName
                }

                MouseArea {
                    anchors.fill: parent
                    anchors.margins: verticalHeaderDelegate.cellPadding / 2
                    acceptedButtons: Qt.LeftButton | Qt.RightButton

                    onPressed: function(event) {
                        if (event.modifiers === Qt.AltModifier) {
                            event.accepted = false
                            return
                        }
                    }

                    onClicked: function(event) {
                        switch (event.button) {
                        case Qt.LeftButton:
                            if (event.modifiers & Qt.ControlModifier)
                                selectionModel.toggleRow(index)
                            else
                                selectionModel.selectRow(index)
                            break
                        case Qt.RightButton:
                            rowMenu.row = index
                            const menu_pos = mapToItem(verticalHeaderView, width + anchors.margins, -anchors.margins)
                            rowMenu.popup(menu_pos)
                            break
                        }
                    }
                }
            }
            Menu {
                id: rowMenu

                property int row: -1

                onOpened: {
                    verticalHeaderSelectionModel.setCurrent(row)
                }

                onClosed: {
                    verticalHeaderSelectionModel.setCurrent()
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
                                tableView.hideRow(rows[i].row)
                            selectionModel.clearSelection()
                        } else {
                            tableView.hideRow(rowMenu.row)
                        }
                    }
                }

                MenuItem {
                    text: qsTr("Show hidden row(s)")
                    icon {
                        source: "icons/show.svg"
                        color: palette.text
                    }
                    enabled: tableView.hiddenRowCount

                    onClicked: {
                        tableView.showHiddenRows()
                        selectionModel.clearSelection()
                    }
                }

                MenuItem {
                    text: qsTr("Reset row reordering")
                    icon {
                        source: "icons/reset_reordering.svg"
                        color: palette.text
                    }

                    onClicked: tableView.resetRowReordering()
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            TableView {
                id: tableView

                property int hiddenColumnCount: 0
                property int hiddenRowCount: 0

                anchors.fill: parent
                clip: true
                columnSpacing: 2
                rowSpacing: 2
                boundsBehavior: Flickable.StopAtBounds
                selectionBehavior: TableView.SelectCells
                selectionMode: TableView.ExtendedSelection
                selectionModel: selectionModel
                interactive: toolbar.panEnabled
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
                    if (selectionModel.hasSelection) {
                        const source_index = selectionModel.selectedIndexes[0]
                        mimeDataProvider.sourceCell = cellAtIndex(source_index)
                        mimeDataProvider.loadSelectedData()
                    } else {
                        const current_index = selectionModel.currentIndex
                        const current_cell = cellAtIndex(current_index)
                        mimeDataProvider.sourceCell = current_cell
                        mimeDataProvider.loadDataFromModel(current_cell, current_index, model)
                    }
                }

                function cutToClipboard()
                {
                    mimeDataProvider.reset()
                    if (selectionModel.hasSelection) {
                        const source_index = selectionModel.selectedIndexes[0]
                        mimeDataProvider.sourceCell = cellAtIndex(source_index)
                        mimeDataProvider.loadSelectedData()
                    } else {
                        const current_index = selectionModel.currentIndex
                        const current_cell = cellAtIndex(current_index)
                        mimeDataProvider.sourceCell = current_cell
                        mimeDataProvider.loadDataFromModel(current_cell, current_index, model)
                    }
                    mimeDataProvider.includeCutData = true
                }

                function pasteFromClipboard()
                {
                    visibleCellsConnection.blockConnection(true)
                    const current_index = selectionModel.currentIndex
                    const current_cell = cellAtIndex(current_index)
                    if (mimeDataProvider.size() === 1) {
                        if (selectionModel.hasSelection) {
                            for (let i in selectionModel.selectedIndexes)
                                mimeDataProvider.saveDataToModel(0, selectionModel.selectedIndexes[i], model)
                        } else {
                            const old_cell = mimeDataProvider.cellAt(0)
                            const old_index = tableView.index(old_cell.y, old_cell.x)
                            const new_x = old_cell.x + current_cell.x - mimeDataProvider.sourceCell.x
                            const new_y = old_cell.y + current_cell.y - mimeDataProvider.sourceCell.y
                            mimeDataProvider.saveDataToModel(0, index(new_y, new_x), model)
                        }
                    } else if (mimeDataProvider.size() > 1) {
                        for (let i = 0; i < mimeDataProvider.size(); ++i) {
                            let cell_i = mimeDataProvider.cellAt(i)
                            cell_i.x += current_cell.x - mimeDataProvider.sourceCell.x
                            cell_i.y += current_cell.y - mimeDataProvider.sourceCell.y
                            const index_i = index(cell_i.y, cell_i.x)
                            mimeDataProvider.saveDataToModel(i, index_i, model)
                        }
                    }
                    if (mimeDataProvider.includeCutData) {
                        for (let i = 0; i < mimeDataProvider.size(); ++i) {
                            const cell_i = mimeDataProvider.cellAt(i)
                            model.clearItemData(index(cell_i.y, cell_i.x))
                        }
                        mimeDataProvider.includeCutData = false
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

                ScrollBar.horizontal: ScrollBar { }
                ScrollBar.vertical: ScrollBar { }

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
                        if (selectionModel.hasSelection)
                            model.clearItemData(selectionModel.selectedIndexes)
                        else
                            model.clearItemData(selectionModel.currentIndex)
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

                    function updateViewArea()
                    {
                        visibleCellsConnection.blockConnection(true)
                        const topRow = tableView.topRow
                        const bottomRow = tableView.bottomRow
                        const leftColumn = tableView.leftColumn
                        const rightColumn = tableView.rightColumn
                        SpreadModel.update(topRow, bottomRow, leftColumn, rightColumn)
                        visibleCellsConnection.blockConnection(false)
                    }

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
                        hadSelection = selectionModel.hasSelection
                        if (!hadSelection)
                            selectionModel.select(index, ItemSelectionModel.Select)
                        if (!selectionModel.isSelected(index))
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
                            selectionModel.clearSelection()
                        hadSelection = false
                        dragCell = Qt.point(-1, -1)
                    }
                }
            }

            DropArea {
                id: dropArea

                property point dropCell: Qt.point(-1, -1)

                anchors.fill: tableView
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

                    tableView.model.clearItemData(selectionModel.selectedIndexes)
                    for (let i = 0; i < mimeDataProvider.size(); ++i) {
                        let cell = mimeDataProvider.cellAt(i)
                        cell.x += dropCell.x - dragArea.dragCell.x
                        cell.y += dropCell.y - dragArea.dragCell.y
                        const index = tableView.index(cell.y, cell.x)
                        mimeDataProvider.saveDataToModel(i, index, tableView.model)
                    }
                    mimeDataProvider.reset()
                    selectionModel.clearSelection()

                    const drop_index = tableView.index(dropCell.y, dropCell.x)
                    selectionModel.setCurrentIndex(drop_index, ItemSelectionModel.Current)

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
                    for (let i in selectionModel.selectedIndexes) {
                        const old_index = selectionModel.selectedIndexes[i]
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

    SelectionRectangle {
        id: selectionRectangle
        target: tableView
        selectionMode: SelectionRectangle.Auto

        topLeftHandle: Rectangle {
            width: 20
            height: 20
            radius: 10
            color: Qt.styleHints.colorScheme === Qt.Light ? palette.highlight.lighter(1.4)
                                                          : palette.highlight.darker(1.4)
            visible: SelectionRectangle.control.active
        }

        bottomRightHandle: Rectangle {
            width: 20
            height: 20
            radius: 10
            color: Qt.styleHints.colorScheme === Qt.Light ? palette.highlight.lighter(1.4)
                                                          : palette.highlight.darker(1.4)
            visible: SelectionRectangle.control.active
        }
    }

    SpreadSelectionModel {
        id: selectionModel
        behavior: SpreadSelectionModel.SelectCells
    }

    SpreadMimeDataProvider {
        id: mimeDataProvider

        property bool includeCutData: false
        property point sourceCell: Qt.point(-1, -1)

        function loadSelectedData()
        {
            for (let i in selectionModel.selectedIndexes) {
                const index = selectionModel.selectedIndexes[i]
                const cell = tableView.cellAtIndex(index)
                loadDataFromModel(cell, index, tableView.model)
            }
        }

        function resetProvider()
        {
            sourceCell = Qt.point(-1, -1)
            includeCutData = false
            reset()
        }
    }

    HelpDialog {
        id: helpDialog
        anchors.centerIn: parent
    }
}
