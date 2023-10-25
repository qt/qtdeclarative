// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtTest
import QtQuick.Controls
import Qt.labs.qmlmodels

TestCase {
    id: testCase
    width: 200
    height: 200
    visible: true
    when: windowShown
    name: "SelectionRectangle"

    property real cellWidth: 50
    property real cellHeight: 20
    property Item topLeftHandle: null
    property Item bottomRightHandle: null
    property bool handleWasDragged: false

    Component {
        id: defaultSelectionRectangle

        SelectionRectangle {}
    }

    Component {
        id: topLeftHandleComp
        Rectangle {
            id: handle
            width: 28
            height: width
            radius: width / 2
            property bool dragging: SelectionRectangle.dragging
            property Item control: SelectionRectangle.control
            border.width: 1
            border.color: "red"
            visible: SelectionRectangle.control.active

            SelectionRectangle.onDraggingChanged: {
                if (SelectionRectangle.dragging)
                    testCase.handleWasDragged = true
            }

            Component.onCompleted: testCase.topLeftHandle = handle
            Component.onDestruction: testCase.topLeftHandle = null
        }
    }

    Component {
        id: bottomRightHandleComp
        Rectangle {
            id: handle
            width: 28
            height: width
            radius: width / 2
            property bool dragging: SelectionRectangle.dragging
            property Item control: SelectionRectangle.control
            border.width: 1
            border.color: "green"
            visible: SelectionRectangle.control.active

            SelectionRectangle.onDraggingChanged: {
                if (SelectionRectangle.dragging)
                    testCase.handleWasDragged = true
            }

            Component.onCompleted: testCase.bottomRightHandle = handle
            Component.onDestruction: testCase.bottomRightHandle = null
        }
    }

    Component {
        id: tableviewComp
        TableView {
            id: tableView
            clip: true
            anchors.fill: parent

            model: TableModel {
                TableModelColumn { display: "c1" }
                TableModelColumn { display: "c2" }
                TableModelColumn { display: "c3" }
                TableModelColumn { display: "c4" }
                TableModelColumn { display: "c5" }
                TableModelColumn { display: "c6" }
                TableModelColumn { display: "c7" }
                TableModelColumn { display: "c8" }
                rows: [
                    { "c1": "v1", "c2":"v2", "c3":"v3", "c4": "v4", "c5": "v5", "c6":"v6", "c7":"v7", "c8": "v8" },
                    { "c1": "v1", "c2":"v2", "c3":"v3", "c4": "v4", "c5": "v5", "c6":"v6", "c7":"v7", "c8": "v8" },
                    { "c1": "v1", "c2":"v2", "c3":"v3", "c4": "v4", "c5": "v5", "c6":"v6", "c7":"v7", "c8": "v8" },
                    { "c1": "v1", "c2":"v2", "c3":"v3", "c4": "v4", "c5": "v5", "c6":"v6", "c7":"v7", "c8": "v8" },
                    { "c1": "v1", "c2":"v2", "c3":"v3", "c4": "v4", "c5": "v5", "c6":"v6", "c7":"v7", "c8": "v8" },
                    { "c1": "v1", "c2":"v2", "c3":"v3", "c4": "v4", "c5": "v5", "c6":"v6", "c7":"v7", "c8": "v8" },
                    { "c1": "v1", "c2":"v2", "c3":"v3", "c4": "v4", "c5": "v5", "c6":"v6", "c7":"v7", "c8": "v8" },
                    { "c1": "v1", "c2":"v2", "c3":"v3", "c4": "v4", "c5": "v5", "c6":"v6", "c7":"v7", "c8": "v8" },
                    { "c1": "v1", "c2":"v2", "c3":"v3", "c4": "v4", "c5": "v5", "c6":"v6", "c7":"v7", "c8": "v8" },
                    { "c1": "v1", "c2":"v2", "c3":"v3", "c4": "v4", "c5": "v5", "c6":"v6", "c7":"v7", "c8": "v8" },
                    { "c1": "v1", "c2":"v2", "c3":"v3", "c4": "v4", "c5": "v5", "c6":"v6", "c7":"v7", "c8": "v8" },
                    { "c1": "v1", "c2":"v2", "c3":"v3", "c4": "v4", "c5": "v5", "c6":"v6", "c7":"v7", "c8": "v8" },
                    { "c1": "v1", "c2":"v2", "c3":"v3", "c4": "v4", "c5": "v5", "c6":"v6", "c7":"v7", "c8": "v8" },
                    { "c1": "v1", "c2":"v2", "c3":"v3", "c4": "v4", "c5": "v5", "c6":"v6", "c7":"v7", "c8": "v8" },
                    { "c1": "v1", "c2":"v2", "c3":"v3", "c4": "v4", "c5": "v5", "c6":"v6", "c7":"v7", "c8": "v8" }
                ]
            }

            delegate: Rectangle {
                required property bool selected
                implicitWidth: cellWidth
                implicitHeight: cellHeight
                color: selected ? "lightblue" : "gray"
                Text { text: row + "," + column }
            }

            selectionModel: ItemSelectionModel {
                model: tableView.model
            }

            property alias selectionRectangle: selectionRectangle
            SelectionRectangle {
                id: selectionRectangle
                target: tableView
            }
        }

    }

    Component {
        id: headerviewComp
        HorizontalHeaderView {
            id: headerView
            clip: true
            anchors.fill: parent

            model: TableModel {
                TableModelColumn { display: "c1" }
                TableModelColumn { display: "c2" }
                TableModelColumn { display: "c3" }
                TableModelColumn { display: "c4" }
                TableModelColumn { display: "c5" }
                TableModelColumn { display: "c6" }
                TableModelColumn { display: "c7" }
                TableModelColumn { display: "c8" }
                rows: [
                    { "c1": "v1", "c2":"v2", "c3":"v3", "c4": "v4", "c5": "v5", "c6":"v6", "c7":"v7", "c8": "v8" }
                ]
            }

            delegate: Rectangle {
                required property bool selected
                implicitWidth: cellWidth
                implicitHeight: cellHeight
                color: selected ? "lightblue" : "gray"
                Text { text: row + "," + column }
            }

            selectionModel: ItemSelectionModel { }

            property alias selectionRectangle: selectionRectangle
            SelectionRectangle {
                id: selectionRectangle
                target: headerView
            }
        }

    }

    Component {
        id: signalSpy
        SignalSpy { }
    }

    function test_defaults() {
        failOnWarning(/.?/)

        let control = createTemporaryObject(defaultSelectionRectangle, testCase)
        verify(control)
    }

    function test_set_target() {
        let tableView = createTemporaryObject(tableviewComp, testCase)
        verify(tableView)
        let selectionRectangle = tableView.selectionRectangle
        verify(selectionRectangle)

        compare(selectionRectangle.target, tableView)

        selectionRectangle.target = null
        compare(selectionRectangle.target, null)

        selectionRectangle.target = tableView
        compare(selectionRectangle.target, tableView)
    }

    function test_set_selectionMode() {
        let tableView = createTemporaryObject(tableviewComp, testCase)
        verify(tableView)
        let selectionRectangle = tableView.selectionRectangle
        verify(selectionRectangle)

        // Default selection mode should be Auto
        compare(selectionRectangle.selectionMode, SelectionRectangle.Auto)

        selectionRectangle.selectionMode = SelectionRectangle.Drag
        compare(selectionRectangle.selectionMode, SelectionRectangle.Drag)

        selectionRectangle.selectionMode = SelectionRectangle.PressAndHold
        compare(selectionRectangle.selectionMode, SelectionRectangle.PressAndHold)

        selectionRectangle.selectionMode = SelectionRectangle.Auto
        compare(selectionRectangle.selectionMode, SelectionRectangle.Auto)
    }

    function test_set_handles() {
        let tableView = createTemporaryObject(tableviewComp, testCase)
        verify(tableView)
        let selectionRectangle = tableView.selectionRectangle
        verify(selectionRectangle)

        selectionRectangle.topLeftHandle = null
        compare(selectionRectangle.topLeftHandle, null)

        selectionRectangle.bottomRightHandle = null
        compare(selectionRectangle.bottomRightHandle, null)

        selectionRectangle.topLeftHandle = topLeftHandleComp
        compare(selectionRectangle.topLeftHandle, topLeftHandleComp)

        selectionRectangle.bottomRightHandle = bottomRightHandleComp
        compare(selectionRectangle.bottomRightHandle, bottomRightHandleComp)
    }

    function test_drag_data() {
        return [
            { tag: "resize enabled (tableview)", resizeEnabled: true, viewComp: tableviewComp },
            { tag: "resize disabled (tableview)", resizeEnabled: false, viewComp: tableviewComp },
            { tag: "resize enabled (headerview)", resizeEnabled: true, viewComp: headerviewComp },
            { tag: "resize disabled (headerview)", resizeEnabled: false, viewComp: headerviewComp },
        ]
    }

    function test_drag(data) {
        let tableView = createTemporaryObject(data.viewComp, testCase)
        verify(tableView)
        let selectionRectangle = tableView.selectionRectangle
        verify(selectionRectangle)

        // Check that we can start a selection from the middle of a cell, even
        // if a drag or tap on the edge of the cell would resize it.
        tableView.resizableRows = data.resizeEnabled
        tableView.resizableColumns = data.resizeEnabled

        selectionRectangle.selectionMode = SelectionRectangle.Drag

        let activeSpy = signalSpy.createObject(selectionRectangle, {target: selectionRectangle, signalName: "activeChanged"})
        let draggingSpy = signalSpy.createObject(selectionRectangle, {target: selectionRectangle, signalName: "draggingChanged"})
        verify(activeSpy.valid)
        verify(draggingSpy.valid)

        verify(!tableView.selectionModel.hasSelection)
        mouseDrag(tableView, 1, 1, (cellWidth * 2) - 2, 1, Qt.LeftButton)
        verify(tableView.selectionModel.hasSelection)
        compare(tableView.selectionModel.selectedIndexes.length, 2)
        verify(tableView.selectionModel.isSelected(tableView.selectionModel.model.index(0, 0)))
        verify(tableView.selectionModel.isSelected(tableView.selectionModel.model.index(0, 1)))

        compare(activeSpy.count, 1)
        compare(draggingSpy.count, 2)

        // Remove selection
        mouseClick(tableView, tableView.width - 1, tableView.height - 1, Qt.LeftButton)
        verify(!tableView.selectionModel.hasSelection)
        compare(draggingSpy.count, 2)
        compare(activeSpy.count, 2)

        // Ensure that a press and hold doesn't start a selection
        mousePress(tableView, 1, 1, Qt.LeftButton)
        mousePress(tableView, 1, 1, Qt.LeftButton, Qt.NoModifier, 1000)
        verify(!tableView.selectionModel.hasSelection)
    }

    function test_tableView_singleSelection_data() {
        return [
            { viewComp: tableviewComp },
            { viewComp: headerviewComp },
        ]
    }

    function test_tableView_singleSelection(data) {
        let tableView = createTemporaryObject(data.viewComp, testCase)
        verify(tableView)
        let selectionRectangle = tableView.selectionRectangle
        verify(selectionRectangle)

        selectionRectangle.selectionMode = SelectionRectangle.Drag
        tableView.selectionMode = TableView.SingleSelection

        // Try to select two cells by dragging. Only one cell should be selected.
        verify(!tableView.selectionModel.hasSelection)
        mouseDrag(tableView, 1, 1, (cellWidth * 2) - 2, 1, Qt.LeftButton)
        verify(tableView.selectionModel.hasSelection)
        compare(tableView.selectionModel.selectedIndexes.length, 1)
        verify(tableView.selectionModel.isSelected(tableView.selectionModel.model.index(0, 0)))

        // A control click should clear the current selection and select a new cell
        mouseClick(tableView, (cellWidth * 2) - 1, 1, Qt.LeftButton, Qt.ControlModifier)
        compare(tableView.selectionModel.selectedIndexes.length, 1)
        verify(tableView.selectionModel.isSelected(tableView.selectionModel.model.index(0, 1)))

        // A shift click is a no-op, and doesn't change the current selection
        mouseClick(tableView, 1, 1, Qt.LeftButton, Qt.ShiftModifier)
        compare(tableView.selectionModel.selectedIndexes.length, 1)
        verify(tableView.selectionModel.isSelected(tableView.selectionModel.model.index(0, 1)))
    }

    function test_tableView_contiguousSelection() {
        let tableView = createTemporaryObject(tableviewComp, testCase)
        verify(tableView)
        let selectionRectangle = tableView.selectionRectangle
        verify(selectionRectangle)

        selectionRectangle.selectionMode = SelectionRectangle.Drag
        tableView.selectionMode = TableView.ContiguousSelection

        // Select two cells by dragging
        verify(!tableView.selectionModel.hasSelection)
        mouseDrag(tableView, 1, 1, (cellWidth * 2) - 2, 1, Qt.LeftButton)
        verify(tableView.selectionModel.hasSelection)
        compare(tableView.selectionModel.selectedIndexes.length, 2)
        verify(tableView.selectionModel.isSelected(tableView.model.index(0, 0)))
        verify(tableView.selectionModel.isSelected(tableView.model.index(0, 1)))

        // A shift click should extend the selection
        mouseClick(tableView, (cellWidth * 4) - 3, 1, Qt.LeftButton, Qt.ShiftModifier)
        compare(tableView.selectionModel.selectedIndexes.length, 4)
        verify(tableView.selectionModel.isSelected(tableView.model.index(0, 0)))
        verify(tableView.selectionModel.isSelected(tableView.model.index(0, 1)))
        verify(tableView.selectionModel.isSelected(tableView.model.index(0, 2)))
        verify(tableView.selectionModel.isSelected(tableView.model.index(0, 3)))

        // A shift click closer to the first selected cell should shrink it again
        mouseClick(tableView, (cellWidth * 3) - 2, 1, Qt.LeftButton, Qt.ShiftModifier)
        compare(tableView.selectionModel.selectedIndexes.length, 3)
        verify(tableView.selectionModel.isSelected(tableView.model.index(0, 0)))
        verify(tableView.selectionModel.isSelected(tableView.model.index(0, 1)))
        verify(tableView.selectionModel.isSelected(tableView.model.index(0, 2)))

        // A control click should clear the selection, and select a new cell
        mouseClick(tableView, 1, (cellHeight * 2) - 1, Qt.LeftButton, Qt.ControlModifier)
        compare(tableView.selectionModel.selectedIndexes.length, 1)
        verify(tableView.selectionModel.isSelected(tableView.model.index(1, 0)))

        // A control drag should clear the selection, and select new cells
        mouseDrag(tableView, 1, 1, (cellWidth * 2) - 2, 1, Qt.LeftButton, Qt.ControlModifier)
        verify(tableView.selectionModel.hasSelection)
        compare(tableView.selectionModel.selectedIndexes.length, 2)
        verify(tableView.selectionModel.isSelected(tableView.model.index(0, 0)))
        verify(tableView.selectionModel.isSelected(tableView.model.index(0, 1)))
    }

    function test_tableView_extendedSelection() {
        let tableView = createTemporaryObject(tableviewComp, testCase)
        verify(tableView)
        let selectionRectangle = tableView.selectionRectangle
        verify(selectionRectangle)

        selectionRectangle.selectionMode = SelectionRectangle.Drag
        // ExtendedSelection should be the default selection mode
        compare(tableView.selectionMode, TableView.ExtendedSelection)

        // Select two cells by dragging
        verify(!tableView.selectionModel.hasSelection)
        mouseDrag(tableView, 1, 1, (cellWidth * 2) - 2, 1, Qt.LeftButton)
        verify(tableView.selectionModel.hasSelection)
        compare(tableView.selectionModel.selectedIndexes.length, 2)
        verify(tableView.selectionModel.isSelected(tableView.model.index(0, 0)))
        verify(tableView.selectionModel.isSelected(tableView.model.index(0, 1)))

        // A shift click should extend the selection
        mouseClick(tableView, (cellWidth * 3) - 2, 1, Qt.LeftButton, Qt.ShiftModifier)
        compare(tableView.selectionModel.selectedIndexes.length, 3)
        verify(tableView.selectionModel.isSelected(tableView.model.index(0, 0)))
        verify(tableView.selectionModel.isSelected(tableView.model.index(0, 1)))
        verify(tableView.selectionModel.isSelected(tableView.model.index(0, 2)))

        // A control click should add a new cell to the selection
        mouseClick(tableView, 1, (cellHeight * 2) - 1, Qt.LeftButton, Qt.ControlModifier)
        compare(tableView.selectionModel.selectedIndexes.length, 4)
        verify(tableView.selectionModel.isSelected(tableView.model.index(0, 0)))
        verify(tableView.selectionModel.isSelected(tableView.model.index(0, 1)))
        verify(tableView.selectionModel.isSelected(tableView.model.index(0, 2)))
        verify(tableView.selectionModel.isSelected(tableView.model.index(1, 0)))

        // A shift click should further extend the selection from the last cell selected
        mouseClick(tableView, (cellWidth * 3) - 2, (cellHeight * 2) - 1, Qt.LeftButton, Qt.ShiftModifier)
        compare(tableView.selectionModel.selectedIndexes.length, 6)
        for (let r = 0; r < 2; ++r)
            for (let c = 0; c < 3; ++c)
                verify(tableView.selectionModel.isSelected(tableView.model.index(r, c)))
    }

    function test_handle_position() {
        // Check that the handles end up at the corner of the selection
        // even if if we resize some rows and column while they have
        // been flicked out of the viewport.
        let tableView = createTemporaryObject(tableviewComp, testCase)
        verify(tableView)

        let selectionRectangle = tableView.selectionRectangle
        verify(selectionRectangle)
        selectionRectangle.topLeftHandle = topLeftHandleComp
        selectionRectangle.bottomRightHandle = bottomRightHandleComp
        selectionRectangle.selectionMode = SelectionRectangle.Drag
        tableView.animate = false
        verify(waitForItemPolished(tableView))

        // Select a cell
        let selectedCell = Qt.point(1, 1)
        let selectedItem = tableView.itemAtCell(selectedCell)
        verify(selectedItem)
        mouseDrag(tableView.contentItem, selectedItem.x, selectedItem.y, 10, 10, Qt.LeftButton)
        compare(tableView.selectionModel.selectedIndexes.length, 1)
        verify(topLeftHandle)
        verify(bottomRightHandle)

        // Check that the handles are placed at the corners of the selected cell
        compare(topLeftHandle.x, selectedItem.x - (topLeftHandle.width / 2))
        compare(topLeftHandle.y, selectedItem.y - (topLeftHandle.height / 2))
        compare(bottomRightHandle.x, selectedItem.x + selectedItem.width - (bottomRightHandle.width / 2))
        compare(bottomRightHandle.y, selectedItem.y + selectedItem.height - (bottomRightHandle.height / 2))

        // Resize some of the rows and columns that will
        // affect the position of the handles.
        tableView.setColumnWidth(0, cellWidth + 5)
        tableView.setRowHeight(0, cellHeight + 5)
        tableView.setColumnWidth(1, cellWidth + 5)
        tableView.setRowHeight(1, cellHeight + 5)
        verify(waitForItemPolished(tableView))

        // Check that the handles are still placed at
        // the corners of the selected cell.
        compare(topLeftHandle.x, selectedItem.x - (topLeftHandle.width / 2))
        compare(topLeftHandle.y, selectedItem.y - (topLeftHandle.height / 2))
        compare(bottomRightHandle.x, selectedItem.x + selectedItem.width - (bottomRightHandle.width / 2))
        compare(bottomRightHandle.y, selectedItem.y + selectedItem.height - (bottomRightHandle.height / 2))

        // Move the selected cell out of the
        // viewport (together with the selection handles).
        tableView.positionViewAtCell(Qt.point(4, 4), TableView.AlignLeft | TableView.AlignTop)
        verify(waitForItemPolished(tableView))

        // Resize some of the rows and columns that will
        // affect the position of the handles.
        tableView.setColumnWidth(0, cellWidth + 10)
        tableView.setRowHeight(0, cellHeight + 10)
        tableView.setColumnWidth(1, cellWidth + 10)
        tableView.setRowHeight(1, cellHeight + 10)
        verify(waitForItemPolished(tableView))

        // Flick the selected cell back into the viewport
        tableView.positionViewAtCell(Qt.point(0, 0), TableView.AlignLeft | TableView.AlignTop)
        verify(waitForItemPolished(tableView))

        // Check that the handles are still placed around the selected cell
        selectedItem = tableView.itemAtCell(selectedCell)
        verify(selectedItem)
        verify(topLeftHandle)
        verify(bottomRightHandle)

        compare(topLeftHandle.x, selectedItem.x - (topLeftHandle.width / 2))
        compare(topLeftHandle.y, selectedItem.y - (topLeftHandle.height / 2))
        compare(bottomRightHandle.x, selectedItem.x + selectedItem.width - (bottomRightHandle.width / 2))
        compare(bottomRightHandle.y, selectedItem.y + selectedItem.height - (bottomRightHandle.height / 2))

        // Remove the selection, and check that the handles end up hidden
        mouseClick(tableView, tableView.width - 1, tableView.height - 1, Qt.LeftButton)
        verify(!tableView.selectionModel.hasSelection)
        verify(!topLeftHandle.visible)
        verify(!bottomRightHandle.visible)
    }

// TODO: enable this test when mouseDrag sends modifiers for all mouse events
// (including mouseMove)
//    function test_multi_selection() {
//        let tableView = createTemporaryObject(tableviewComp, testCase)
//        verify(tableView)
//        let selectionRectangle = tableView.selectionRectangle
//        verify(selectionRectangle)
//        verify(!tableView.selectionModel.hasSelection)

//        selectionRectangle.selectionMode = SelectionRectangle.Drag

//        mouseDrag(tableView, 1, 1, (cellWidth * 2) - 2, 1, Qt.LeftButton)
//        verify(tableView.selectionModel.hasSelection)
//        compare(tableView.selectionModel.selectedIndexes.length, 2)
//        verify(tableView.selectionModel.isSelected(tableView.model.index(0, 0)))
//        verify(tableView.selectionModel.isSelected(tableView.model.index(0, 1)))

//        // Hold down control, and drag again to do a multi-selection
//        mouseDrag(tableView, 1, cellHeight + 5, (cellWidth * 2) - 2, 1, Qt.LeftButton, Qt.ControlModifier)
//        verify(tableView.selectionModel.hasSelection)
//        compare(tableView.selectionModel.selectedIndexes.length, 4)
//        verify(tableView.selectionModel.isSelected(tableView.model.index(0, 0)))
//        verify(tableView.selectionModel.isSelected(tableView.model.index(0, 1)))
//        verify(tableView.selectionModel.isSelected(tableView.model.index(1, 0)))
//        verify(tableView.selectionModel.isSelected(tableView.model.index(1, 1)))
//    }

    function test_pressAndHold_data() {
        return [
            { tag: "resize enabled", resizeEnabled: true },
            { tag: "resize disabled", resizeEnabled: false },
        ]
    }

    function test_pressAndHold(data) {
        let tableView = createTemporaryObject(tableviewComp, testCase)
        verify(tableView)
        let selectionRectangle = tableView.selectionRectangle
        verify(selectionRectangle)

        // Check that we can start a selection from the middle of a cell, even
        // if a drag or tap on the edge of the cell would resize it.
        tableView.resizableRows = data.resizeEnabled
        tableView.resizableColumns = data.resizeEnabled

        selectionRectangle.selectionMode = SelectionRectangle.PressAndHold

        let activeSpy = signalSpy.createObject(selectionRectangle, {target: selectionRectangle, signalName: "activeChanged"})
        let draggingSpy = signalSpy.createObject(selectionRectangle, {target: selectionRectangle, signalName: "draggingChanged"})
        verify(activeSpy.valid)
        verify(draggingSpy.valid)

        verify(!tableView.selectionModel.hasSelection)
        // Do a press and hold
        mousePress(tableView, 1, 1, Qt.LeftButton)
        mousePress(tableView, 1, 1, Qt.LeftButton, Qt.NoModifier, 1000)
        verify(tableView.selectionModel.hasSelection)
        compare(tableView.selectionModel.selectedIndexes.length, 1)
        verify(tableView.selectionModel.isSelected(tableView.model.index(0, 0)))

        compare(draggingSpy.count, 0)
        compare(activeSpy.count, 1)

        // Remove selection
        mouseClick(tableView, 1, 1, Qt.LeftButton)
        verify(!tableView.selectionModel.hasSelection)
        compare(draggingSpy.count, 0)
        compare(activeSpy.count, 2)

        // Ensure that a drag doesn't start a selection
        mouseDrag(tableView, 1, 1, (cellWidth * 2) - 2, 1, Qt.LeftButton)
        verify(!tableView.selectionModel.hasSelection)
    }

    function test_pressAndHold_on_top_of_handle() {
        let tableView = createTemporaryObject(tableviewComp, testCase)
        verify(tableView)
        let selectionRectangle = tableView.selectionRectangle
        verify(selectionRectangle)

        selectionRectangle.selectionMode = SelectionRectangle.PressAndHold

        let activeSpy = signalSpy.createObject(selectionRectangle, {target: selectionRectangle, signalName: "activeChanged"})
        let draggingSpy = signalSpy.createObject(selectionRectangle, {target: selectionRectangle, signalName: "draggingChanged"})
        verify(activeSpy.valid)
        verify(draggingSpy.valid)

        verify(!tableView.selectionModel.hasSelection)
        // Do a press and hold
        mousePress(tableView, 1, 1, Qt.LeftButton)
        mouseRelease(tableView, 1, 1, Qt.LeftButton, Qt.NoModifier, 2000)
        verify(tableView.selectionModel.hasSelection)
        compare(tableView.selectionModel.selectedIndexes.length, 1)
        verify(tableView.selectionModel.isSelected(tableView.model.index(0, 0)))

        compare(draggingSpy.count, 0)
        compare(activeSpy.count, 1)

        // Do another press and hold on top the part of the bottom right handle that
        // also covers cell 1, 1. Without any handles, this would start a new selection
        // on top of that cell. But when the handles are in front, they should block it.
        mousePress(tableView, cellWidth + 1, cellHeight + 1, Qt.LeftButton)
        mouseRelease(tableView, cellWidth + 1, cellHeight + 1, Qt.LeftButton, Qt.NoModifier, 2000)
        compare(tableView.selectionModel.selectedIndexes.length, 1)
        verify(tableView.selectionModel.isSelected(tableView.model.index(0, 0)))
    }

    function test_handleDragTopLeft() {
        let tableView = createTemporaryObject(tableviewComp, testCase)
        verify(tableView)
        let selectionRectangle = tableView.selectionRectangle
        verify(selectionRectangle)

        selectionRectangle.selectionMode = SelectionRectangle.Drag

        verify(!tableView.selectionModel.hasSelection)
        // Select four cells in the middle
        mouseDrag(tableView, cellWidth + 1, cellHeight + 1, (cellWidth * 2) - 2, (cellHeight * 2) - 2, Qt.LeftButton)
        compare(tableView.selectionModel.selectedIndexes.length, 4)
        for (var x = 1; x < 3; ++x) {
           for (var y = 1; y < 3; ++y) {
               verify(tableView.selectionModel.isSelected(tableView.model.index(x, y)))
           }
        }

        // Drag on the top left handle, so that the selection extends to cell 0, 0
        mouseDrag(tableView, cellWidth, cellHeight, -cellWidth / 2, -cellHeight / 2, Qt.LeftButton)
        compare(tableView.selectionModel.selectedIndexes.length, 9)
        for (x = 0; x < 3; ++x) {
           for (y = 0; y < 3; ++y) {
               verify(tableView.selectionModel.isSelected(tableView.model.index(x, y)))
           }
        }
    }

    function test_handleDragBottomRight_shrink() {
        let tableView = createTemporaryObject(tableviewComp, testCase)
        verify(tableView)
        let selectionRectangle = tableView.selectionRectangle
        verify(selectionRectangle)

        selectionRectangle.selectionMode = SelectionRectangle.Drag

        verify(!tableView.selectionModel.hasSelection)
        // Select four cells in the middle
        mouseDrag(tableView, cellWidth + 1, cellHeight + 1, (cellWidth * 2) - 2, (cellHeight * 2) - 2, Qt.LeftButton)
        compare(tableView.selectionModel.selectedIndexes.length, 4)
        for (var x = 1; x < 3; ++x) {
           for (var y = 1; y < 3; ++y) {
               verify(tableView.selectionModel.isSelected(tableView.model.index(x, y)))
           }
        }

        // Drag on the bottom right handle, so that the selection shrinks to cell 1, 1
        mouseDrag(tableView, (cellWidth * 3) - 1, (cellHeight * 3) - 1, -cellWidth, -cellHeight, Qt.LeftButton)
        compare(tableView.selectionModel.selectedIndexes.length, 1)
        verify(tableView.selectionModel.isSelected(tableView.model.index(1, 1)))
    }

    function test_handleDragBottomRight_expand() {
        let tableView = createTemporaryObject(tableviewComp, testCase)
        verify(tableView)
        let selectionRectangle = tableView.selectionRectangle
        verify(selectionRectangle)

        selectionRectangle.selectionMode = SelectionRectangle.Drag

        verify(!tableView.selectionModel.hasSelection)
        // Select four cells in the middle
        mouseDrag(tableView, cellWidth + 1, cellHeight + 1, (cellWidth * 2) - 2, (cellHeight * 2) - 2, Qt.LeftButton)
        compare(tableView.selectionModel.selectedIndexes.length, 4)
        for (var x = 1; x < 3; ++x) {
           for (var y = 1; y < 3; ++y) {
               verify(tableView.selectionModel.isSelected(tableView.model.index(x, y)))
           }
        }

        // Drag on the bottom right handle, so that the selection expands to cell 9 cells
        mouseDrag(tableView, cellWidth * 3, cellHeight * 3, 10, 10, Qt.LeftButton)
        compare(tableView.selectionModel.selectedIndexes.length, 9)
        for (x = 1; x < 4; ++x) {
           for (y = 1; y < 4; ++y) {
               verify(tableView.selectionModel.isSelected(tableView.model.index(x, y)))
           }
        }
    }

}
