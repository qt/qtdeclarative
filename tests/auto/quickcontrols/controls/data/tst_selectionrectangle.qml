// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
        id: delegateWithTapHandlerComp
        Rectangle {
            implicitWidth: 100
            implicitHeight: 50
            color: selected ? "lightblue" : "transparent"
            border.width: 1
            required property bool selected

            TapHandler {
                // This tap handler will block the tap handler in
                // QQuickTableView from being called.
            }
        }
    }

    TableModel {
        id: tableModel
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

    Component {
        id: tableviewComp
        TableView {
            id: tableView
            clip: true
            anchors.fill: parent

            model: tableModel

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
        id: noDelegateTableViewComp
        TableView {
            id: tableView
            anchors.fill: parent
            interactive: false
            selectionModel: itemSelectionModel

            property alias selectionRectangle: selectionRectangle

            ItemSelectionModel {
                id: itemSelectionModel
                model: tableModel
            }

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

    function init() {
        failOnWarning(/.?/)
    }

    function test_defaults() {
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
            { tag: "resize enabled (tableview with delegate)", resizeEnabled: true, viewComp: tableviewComp, hasDelegate: true },
            { tag: "resize disabled (tableview with delegate)", resizeEnabled: false, viewComp: tableviewComp, hasDelegate: true },
            { tag: "resize enabled (tableview no delegate)", resizeEnabled: true, viewComp: noDelegateTableViewComp, hasDelegate: false },
            { tag: "resize disabled (tableview no delegate)", resizeEnabled: false, viewComp: noDelegateTableViewComp, hasDelegate: false },
            { tag: "resize enabled (headerview)", resizeEnabled: true, viewComp: headerviewComp, hasDelegate: true },
            { tag: "resize disabled (headerview)", resizeEnabled: false, viewComp: headerviewComp, hasDelegate: true },
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
        compare(tableView.selectionModel.hasSelection, data.hasDelegate)
        if (tableView.selectionModel.hasSelection) {
            compare(tableView.selectionModel.selectedIndexes.length, 2)
            verify(tableView.selectionModel.isSelected(tableView.selectionModel.model.index(0, 0)))
            verify(tableView.selectionModel.isSelected(tableView.selectionModel.model.index(0, 1)))

            compare(activeSpy.count, 1)
            compare(draggingSpy.count, 2)
        } else {
            compare(activeSpy.count, 0)
            compare(draggingSpy.count, 0)

            verify(!selectionRectangle.active)
            verify(!topLeftHandle)
            verify(!bottomRightHandle)
        }

        // Remove selection
        mouseClick(tableView, tableView.width - 1, tableView.height - 1, Qt.LeftButton)
        verify(!tableView.selectionModel.hasSelection)
        if (data.hasDelegate) {
            compare(draggingSpy.count, 2)
            compare(activeSpy.count, 2)
        } else {
            compare(draggingSpy.count, 0)
            compare(activeSpy.count, 0)
        }

        // Ensure that a press and hold doesn't start a selection
        mousePress(tableView, 1, 1, Qt.LeftButton)
        mouseRelease(tableView, 1, 1, Qt.LeftButton, Qt.NoModifier, 1000)
        verify(!tableView.selectionModel.hasSelection)
    }

    function test_selectionModeAuto_data() {
        return [
            { tag: "mouse, no acceptedButtons, not interactive", acceptedButtons: Qt.NoButton, interactive: false,
                        device: PointerDevice.Mouse, viewComp: tableviewComp,
                        expectSelectionAfterDrag: true, expectSelectionAfterLongPress: false, expectFlick: false },
            { tag: "mouse, no acceptedButtons, interactive", acceptedButtons: Qt.NoButton, interactive: true,
                        device: PointerDevice.Mouse, viewComp: tableviewComp,
                        expectSelectionAfterDrag: true, expectSelectionAfterLongPress: false, expectFlick: false },
            { tag: "mouse, acceptedButtons left, not interactive", acceptedButtons: Qt.LeftButton, interactive: false,
                        device: PointerDevice.Mouse, viewComp: tableviewComp,
                        expectSelectionAfterDrag: true, expectSelectionAfterLongPress: false, expectFlick: false },
            { tag: "mouse, acceptedButtons left, interactive", acceptedButtons: Qt.LeftButton, interactive: true,
                        device: PointerDevice.Mouse, viewComp: tableviewComp,
                        expectSelectionAfterDrag: false, expectSelectionAfterLongPress: true, expectFlick: true },
            { tag: "mouse, acceptedButtons right, interactive", acceptedButtons: Qt.RightButton, interactive: true,
                        device: PointerDevice.Mouse, viewComp: tableviewComp,
                        expectSelectionAfterDrag: true, expectSelectionAfterLongPress: false, expectFlick: false },
            { tag: "touch, no acceptedButtons, not interactive", acceptedButtons: Qt.NoButton, interactive: false,
                        device: PointerDevice.TouchScreen, viewComp: tableviewComp,
                        expectSelectionAfterDrag: true, expectSelectionAfterLongPress: false, expectFlick: false },
            { tag: "touch, acceptedButtons left, not interactive", acceptedButtons: Qt.LeftButton, interactive: false,
                        device: PointerDevice.TouchScreen, viewComp: tableviewComp,
                        expectSelectionAfterDrag: true, expectSelectionAfterLongPress: false, expectFlick: false },
        ]
    }

    function test_selectionModeAuto(data) {
        let tableView = createTemporaryObject(data.viewComp, testCase)
        verify(tableView)
        let selectionRectangle = tableView.selectionRectangle
        verify(selectionRectangle)

        tableView.acceptedButtons = data.acceptedButtons
        tableView.interactive = data.interactive

        selectionRectangle.selectionMode = SelectionRectangle.Auto

        // Try to select by dragging: does it select or flick?
        verify(!tableView.selectionModel.hasSelection)
        switch (data.device) {
        case PointerDevice.Mouse:
            mouseDrag(tableView, 10, 100, 0, -100, Qt.LeftButton)
            break
        case PointerDevice.TouchScreen:
            let touch = touchEvent(tableView)
            touch.press(0, tableView, 10, 100).commit();
            touch.move(0, tableView, 10, 50).commit();
            touch.move(0, tableView, 10, 0).commit();
            touch.release(0, tableView, 10, 0).commit();
            break
        default:
            fail("test does not yet support other device types")
        }

        compare(tableView.selectionModel.hasSelection, data.expectSelectionAfterDrag)
        if (data.expectFlick)
            tryVerify(function(){ return tableView.contentY > 0 })
        else
            compare(tableView.contentY, 0)

        // Remove selection (if any) by clicking outside of it
        mouseClick(tableView, tableView.width - 1, tableView.height - 1, Qt.LeftButton)
        verify(!tableView.selectionModel.hasSelection)

        // Check whether a press and hold starts a selection
        // (we test this only with mouse, because touch.move and touch.release don't have delay arguments)
        mousePress(tableView, 1, 1, Qt.LeftButton)
        mouseRelease(tableView, 1, 1, Qt.LeftButton, Qt.NoModifier, 1000)
        compare(tableView.selectionModel.hasSelection, data.expectSelectionAfterLongPress)
    }

    function test_tableView_singleSelection_data() {
        return [
            { tag: "tableView no delegate", viewComp: noDelegateTableViewComp, hasDelegate: false },
            { tag: "tableView with delegate", viewComp: tableviewComp, hasDelegate: true },
            { tag: "headerView", viewComp: headerviewComp, hasDelegate: true },
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
        compare(tableView.selectionModel.hasSelection, data.hasDelegate)
        if (tableView.selectionModel.hasSelection) {
            compare(tableView.selectionModel.selectedIndexes.length, 1)
            verify(tableView.selectionModel.isSelected(tableView.selectionModel.model.index(0, 0)))
        }

        // A control click should clear the current selection and select a new cell
        mouseClick(tableView, (cellWidth * 2) - 1, 1, Qt.LeftButton, Qt.ControlModifier)
        compare(tableView.selectionModel.hasSelection, data.hasDelegate)
        if (tableView.selectionModel.hasSelection) {
            compare(tableView.selectionModel.selectedIndexes.length, 1)
            verify(tableView.selectionModel.isSelected(tableView.selectionModel.model.index(0, 1)))
        }

        // A shift click is a no-op, and doesn't change the current selection
        mouseClick(tableView, 1, 1, Qt.LeftButton, Qt.ShiftModifier)
        compare(tableView.selectionModel.hasSelection, data.hasDelegate)
        if (tableView.selectionModel.hasSelection) {
            compare(tableView.selectionModel.selectedIndexes.length, 1)
            verify(tableView.selectionModel.isSelected(tableView.selectionModel.model.index(0, 1)))
        }
    }

    function test_tableView_contiguousSelection_data() {
        return [
            { tag: "Dont startFromCurrent (with delegate)", viewComp: tableviewComp, hasDelegate: true, startFromCurrentIndex: false },
            { tag: "startFromCurrent (with delegate)", viewComp: tableviewComp, hasDelegate: true, startFromCurrentIndex: true },
            { tag: "Dont startFromCurrent (no delegate)", viewComp: noDelegateTableViewComp, hasDelegate: false, startFromCurrentIndex: false },
            { tag: "startFromCurrent (no delegate)", viewComp: noDelegateTableViewComp, hasDelegate: false, startFromCurrentIndex: true },
        ]
    }

    function test_tableView_contiguousSelection(data) {
        let tableView = createTemporaryObject(data.viewComp, testCase)
        verify(tableView)
        let selectionRectangle = tableView.selectionRectangle
        verify(selectionRectangle)

        selectionRectangle.selectionMode = SelectionRectangle.Drag
        tableView.selectionMode = TableView.ContiguousSelection

        if (data.startFromCurrentIndex) {
            // Click on a cell to set current index, but set no selection.
            // A later shift-click should then start a new selection from the
            // current cell.
            mouseClick(tableView, 1, 1, Qt.LeftButton, Qt.NoModifier)
            verify(!tableView.selectionModel.hasSelection)
            compare(tableView.selectionModel.currentIndex, tableView.index(0, 0))
        } else {
            // Start a new selection by dragging on two cells
            mouseDrag(tableView, 1, 1, (cellWidth * 2) - 2, 1, Qt.LeftButton)
            compare(tableView.selectionModel.hasSelection, data.hasDelegate)
            if (tableView.selectionModel.hasSelection) {
                compare(tableView.selectionModel.selectedIndexes.length, 2)
                verify(tableView.selectionModel.isSelected(tableView.model.index(0, 0)))
                verify(tableView.selectionModel.isSelected(tableView.model.index(0, 1)))
            } else {
                verify(!selectionRectangle.active)
                verify(!topLeftHandle)
                verify(!bottomRightHandle)
            }
        }

        // A shift click should extend the selection
        mouseClick(tableView, (cellWidth * 4) - 3, 1, Qt.LeftButton, Qt.ShiftModifier)
        compare(tableView.selectionModel.hasSelection, data.hasDelegate)
        if (tableView.selectionModel.hasSelection) {
            compare(tableView.selectionModel.selectedIndexes.length, 4)
            verify(tableView.selectionModel.isSelected(tableView.model.index(0, 0)))
            verify(tableView.selectionModel.isSelected(tableView.model.index(0, 1)))
            verify(tableView.selectionModel.isSelected(tableView.model.index(0, 2)))
            verify(tableView.selectionModel.isSelected(tableView.model.index(0, 3)))
        } else {
            verify(!selectionRectangle.active)
            verify(!topLeftHandle)
            verify(!bottomRightHandle)
        }

        // A shift click closer to the first selected cell should shrink it again
        mouseClick(tableView, (cellWidth * 3) - 2, 1, Qt.LeftButton, Qt.ShiftModifier)
        compare(tableView.selectionModel.hasSelection, data.hasDelegate)
        if (tableView.selectionModel.hasSelection) {
            compare(tableView.selectionModel.selectedIndexes.length, 3)
            verify(tableView.selectionModel.isSelected(tableView.model.index(0, 0)))
            verify(tableView.selectionModel.isSelected(tableView.model.index(0, 1)))
            verify(tableView.selectionModel.isSelected(tableView.model.index(0, 2)))
        } else {
            verify(!selectionRectangle.active)
            verify(!topLeftHandle)
            verify(!bottomRightHandle)
        }

        // A control click should clear the selection, and select a new cell
        mouseClick(tableView, 1, (cellHeight * 2) - 1, Qt.LeftButton, Qt.ControlModifier)
        compare(tableView.selectionModel.hasSelection, data.hasDelegate)
        if (tableView.selectionModel.hasSelection) {
            compare(tableView.selectionModel.selectedIndexes.length, 1)
            verify(tableView.selectionModel.isSelected(tableView.model.index(1, 0)))
        } else {
            verify(!selectionRectangle.active)
            verify(!topLeftHandle)
            verify(!bottomRightHandle)
        }

        // A control drag should clear the selection, and select new cells
        mouseDrag(tableView, 1, 1, (cellWidth * 2) - 2, 1, Qt.LeftButton, Qt.ControlModifier)
        compare(tableView.selectionModel.hasSelection, data.hasDelegate)
        if (tableView.selectionModel.hasSelection) {
            compare(tableView.selectionModel.selectedIndexes.length, 2)
            verify(tableView.selectionModel.isSelected(tableView.model.index(0, 0)))
            verify(tableView.selectionModel.isSelected(tableView.model.index(0, 1)))
        } else {
            verify(!selectionRectangle.active)
            verify(!topLeftHandle)
            verify(!bottomRightHandle)
        }
    }

    function init_delegate_data_tag() {
        return [
            { tag: "with delegate", viewComp: tableviewComp, hasDelegate: true },
            { tag: "no delegate", viewComp: noDelegateTableViewComp, hasDelegate: false },
        ]
    }

    function test_tableView_extendedSelection_data() {
        return init_delegate_data_tag()
    }

    function test_tableView_extendedSelection(data) {
        let tableView = createTemporaryObject(data.viewComp, testCase)
        verify(tableView)
        let selectionRectangle = tableView.selectionRectangle
        verify(selectionRectangle)

        selectionRectangle.selectionMode = SelectionRectangle.Drag
        // ExtendedSelection should be the default selection mode
        compare(tableView.selectionMode, TableView.ExtendedSelection)

        // Select two cells by dragging
        verify(!tableView.selectionModel.hasSelection)
        mouseDrag(tableView, 1, 1, (cellWidth * 2) - 2, 1, Qt.LeftButton)
        compare(tableView.selectionModel.hasSelection, data.hasDelegate)
        if (tableView.selectionModel.hasSelection) {
            compare(tableView.selectionModel.selectedIndexes.length, 2)
            verify(tableView.selectionModel.isSelected(tableView.model.index(0, 0)))
            verify(tableView.selectionModel.isSelected(tableView.model.index(0, 1)))
        } else {
            verify(!selectionRectangle.active)
            verify(!topLeftHandle)
            verify(!bottomRightHandle)
        }

        // A shift click should extend the selection
        mouseClick(tableView, (cellWidth * 3) - 2, 1, Qt.LeftButton, Qt.ShiftModifier)
        compare(tableView.selectionModel.hasSelection, data.hasDelegate)
        if (tableView.selectionModel.hasSelection) {
            compare(tableView.selectionModel.selectedIndexes.length, 3)
            verify(tableView.selectionModel.isSelected(tableView.model.index(0, 0)))
            verify(tableView.selectionModel.isSelected(tableView.model.index(0, 1)))
            verify(tableView.selectionModel.isSelected(tableView.model.index(0, 2)))
        } else {
            verify(!selectionRectangle.active)
            verify(!topLeftHandle)
            verify(!bottomRightHandle)
        }

        // A control click should add a new cell to the selection
        mouseClick(tableView, 1, (cellHeight * 2) - 1, Qt.LeftButton, Qt.ControlModifier)
        compare(tableView.selectionModel.hasSelection, data.hasDelegate)
        if (tableView.selectionModel.hasSelection) {
            compare(tableView.selectionModel.selectedIndexes.length, 4)
            verify(tableView.selectionModel.isSelected(tableView.model.index(0, 0)))
            verify(tableView.selectionModel.isSelected(tableView.model.index(0, 1)))
            verify(tableView.selectionModel.isSelected(tableView.model.index(0, 2)))
            verify(tableView.selectionModel.isSelected(tableView.model.index(1, 0)))
        } else {
            verify(!selectionRectangle.active)
            verify(!topLeftHandle)
            verify(!bottomRightHandle)
        }

        // A shift click should further extend the selection from the last cell selected
        mouseClick(tableView, (cellWidth * 3) - 2, (cellHeight * 2) - 1, Qt.LeftButton, Qt.ShiftModifier)
        compare(tableView.selectionModel.hasSelection, data.hasDelegate)
        if (tableView.selectionModel.hasSelection) {
            compare(tableView.selectionModel.selectedIndexes.length, 6)
            for (let r = 0; r < 2; ++r)
                for (let c = 0; c < 3; ++c)
                    verify(tableView.selectionModel.isSelected(tableView.model.index(r, c)))
        } else {
            verify(!selectionRectangle.active)
            verify(!topLeftHandle)
            verify(!bottomRightHandle)
        }

        // Shift click the second selection so that it overlaps with the first
        mouseClick(tableView, 1, 1, Qt.LeftButton, Qt.ShiftModifier)
        compare(tableView.selectionModel.hasSelection, data.hasDelegate)
        if (tableView.selectionModel.hasSelection) {
            compare(tableView.selectionModel.selectedIndexes.length, 4)
            verify(tableView.selectionModel.isSelected(tableView.model.index(0, 0)))
            verify(tableView.selectionModel.isSelected(tableView.model.index(0, 1)))
            verify(tableView.selectionModel.isSelected(tableView.model.index(0, 2)))
            verify(tableView.selectionModel.isSelected(tableView.model.index(1, 0)))
        } else {
            verify(!selectionRectangle.active)
            verify(!topLeftHandle)
            verify(!bottomRightHandle)
        }

        // Shift click the selection back again. The first selection on
        // row 0 should still be present, even if the second selection
        // no longer overlaps it.
        mouseClick(tableView, (cellWidth * 3) - 2, (cellHeight * 2) - 1, Qt.LeftButton, Qt.ShiftModifier)
        compare(tableView.selectionModel.hasSelection, data.hasDelegate)
        if (tableView.selectionModel.hasSelection) {
            compare(tableView.selectionModel.selectedIndexes.length, 6)
            for (let r = 0; r < 2; ++r)
                for (let c = 0; c < 3; ++c)
                    verify(tableView.selectionModel.isSelected(tableView.model.index(r, c)))
        } else {
            verify(!selectionRectangle.active)
            verify(!topLeftHandle)
            verify(!bottomRightHandle)
        }
    }

    function test_unselect_click_data() {
        return init_delegate_data_tag()
    }

    function test_unselect_click(data) {
        // Check at a ctrl click on top a selected cell
        // will cause the cell to be unselected.
        let tableView = createTemporaryObject(data.viewComp, testCase)
        verify(tableView)
        let selectionRectangle = tableView.selectionRectangle
        verify(selectionRectangle)

        selectionRectangle.selectionMode = SelectionRectangle.Drag

        // Select some cells
        tableView.selectionModel.select(tableView.index(0, 0), ItemSelectionModel.Select)
        tableView.selectionModel.select(tableView.index(0, 1), ItemSelectionModel.Select)
        tableView.selectionModel.select(tableView.index(0, 3), ItemSelectionModel.Select)
        tableView.selectionModel.select(tableView.index(1, 0), ItemSelectionModel.Select)
        compare(tableView.selectionModel.hasSelection, data.hasDelegate)
        if (tableView.selectionModel.hasSelection) {
            compare(tableView.selectionModel.selectedIndexes.length, 4)
        } else {
            verify(!selectionRectangle.active)
            verify(!topLeftHandle)
            verify(!bottomRightHandle)
        }

        // Do a ctrl-click on top of a selected cell. This
        // should cause the cell to be unselected.
        mouseClick(tableView, cellWidth / 2, cellHeight / 2, Qt.LeftButton, Qt.ControlModifier)
        compare(tableView.selectionModel.hasSelection, data.hasDelegate)
        if (tableView.selectionModel.hasSelection) {
            compare(tableView.selectionModel.selectedIndexes.length, 3)
            verify(!tableView.selectionModel.isSelected(tableView.model.index(0, 0)))
            verify(tableView.selectionModel.isSelected(tableView.model.index(0, 1)))
            verify(tableView.selectionModel.isSelected(tableView.model.index(0, 3)))
            verify(tableView.selectionModel.isSelected(tableView.model.index(1, 0)))
            verify(selectionRectangle.active)
        } else {
            verify(!selectionRectangle.active)
            verify(!topLeftHandle)
            verify(!bottomRightHandle)
        }
    }

    function test_unselect_drag_data() {
        return init_delegate_data_tag()
    }

    function test_unselect_drag(data) {
        // Check at a ctrl drag on top a selected cell
        // will cause the dragged-over cells to be unselected.
        let tableView = createTemporaryObject(data.viewComp, testCase)
        verify(tableView)
        let selectionRectangle = tableView.selectionRectangle
        verify(selectionRectangle)

        selectionRectangle.selectionMode = SelectionRectangle.Drag

        // Select some cells
        tableView.selectionModel.select(tableView.index(0, 0), ItemSelectionModel.Select)
        tableView.selectionModel.select(tableView.index(0, 1), ItemSelectionModel.Select)
        tableView.selectionModel.select(tableView.index(0, 3), ItemSelectionModel.Select)
        tableView.selectionModel.select(tableView.index(1, 0), ItemSelectionModel.Select)
        compare(tableView.selectionModel.hasSelection, data.hasDelegate)
        if (tableView.selectionModel.hasSelection) {
            compare(tableView.selectionModel.selectedIndexes.length, 4)
        } else {
            verify(!selectionRectangle.active)
            verify(!topLeftHandle)
            verify(!bottomRightHandle)
        }

        // Do a ctrl-drag on top of the selected cells. This
        // should cause all the cells to be unselected.
        mousePress(tableView.contentItem, cellWidth / 2, cellHeight / 2, Qt.LeftButton, Qt.ControlModifier)
        mouseMove(tableView.contentItem, cellWidth * 4, cellHeight * 2, 0, Qt.LeftButton, Qt.ControlModifier)
        compare(tableView.selectionModel.selectedIndexes.length, 0)

        // Move the mouse back to cell 2, and release the mouse. Only
        // the top left cells will then be unselected
        mouseMove(tableView.contentItem, (cellWidth * 2) - 1, (cellHeight * 2) - 1, 0, Qt.LeftButton, Qt.ControlModifier)
        mouseRelease(tableView.contentItem, (cellWidth * 2) - 1, (cellHeight * 2) - 1, Qt.LeftButton, Qt.ControlModifier)
        compare(tableView.selectionModel.hasSelection, data.hasDelegate)
        if (tableView.selectionModel.hasSelection) {
            compare(tableView.selectionModel.selectedIndexes.length, 1)
            verify(tableView.selectionModel.isSelected(tableView.model.index(0, 3)))
        } else {
            verify(!selectionRectangle.active)
            verify(!topLeftHandle)
            verify(!bottomRightHandle)
        }
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
        verify(tableView.selectionModel.hasSelection)
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

    function test_delegateWithTapHandler() {
        // Check that we clear the current selection if you start a new
        // mouse drag selection on top of a delegate with a tap handler.
        let tableView = createTemporaryObject(tableviewComp, testCase)
        verify(tableView)

        tableView.delegate = delegateWithTapHandlerComp;
        let selectionRectangle = tableView.selectionRectangle
        verify(selectionRectangle)

        selectionRectangle.selectionMode = SelectionRectangle.Drag
        tableView.selectionMode = TableView.ExtendedSelection

        verify(waitForItemPolished(tableView))

        let item0_0 = tableView.itemAtIndex(tableView.index(0, 0))
        let item1_1 = tableView.itemAtIndex(tableView.index(1, 1))
        verify(item0_0)
        verify(item1_1)

        tableView.selectionModel.select(tableView.index(0, 0), ItemSelectionModel.Select)
        compare(tableView.selectionModel.selectedIndexes.length, 1)
        compare(tableView.selectionModel.selectedIndexes[0], tableView.index(0, 0))

        // A drag should clear the current selection and select a new cell
        mouseDrag(tableView.contentItem, item1_1.x, item1_1.y, 10, 10, Qt.LeftButton)
        compare(tableView.selectionModel.selectedIndexes.length, 1)
        compare(tableView.selectionModel.selectedIndexes[0], tableView.index(1, 1))

        // Verify that a PressAndHold works as well
        selectionRectangle.selectionMode = SelectionRectangle.PressAndHold
        mousePress(tableView, item0_0.x, item0_0.y, Qt.LeftButton)
        mouseRelease(tableView, item0_0.x, item0_0.y, Qt.LeftButton, Qt.NoModifier, 2000)
        compare(tableView.selectionModel.selectedIndexes.length, 1)
        compare(tableView.selectionModel.selectedIndexes[0], tableView.index(0, 0))
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
            { tag: "resize enabled (with delegate)", resizeEnabled: true, viewComp: tableviewComp, hasDelegate: true },
            { tag: "resize disabled (with delegate)", resizeEnabled: false, viewComp: tableviewComp, hasDelegate: true },
            { tag: "resize enabled (no delegate)", resizeEnabled: true, viewComp: noDelegateTableViewComp, hasDelegate: false },
            { tag: "resize disabled (no delegate)", resizeEnabled: false, viewComp: noDelegateTableViewComp, hasDelegate: false },
        ]
    }

    function test_pressAndHold(data) {
        let tableView = createTemporaryObject(data.viewComp, testCase)
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
        mouseRelease(tableView, 1, 1, Qt.LeftButton, Qt.NoModifier, 1000)
        compare(tableView.selectionModel.hasSelection, data.hasDelegate)
        if (tableView.selectionModel.hasSelection) {
            compare(tableView.selectionModel.selectedIndexes.length, 1)
            verify(tableView.selectionModel.isSelected(tableView.model.index(0, 0)))

            compare(draggingSpy.count, 0)
            compare(activeSpy.count, 1)
        } else {
            verify(!selectionRectangle.active)
            verify(!topLeftHandle)
            verify(!bottomRightHandle)

            compare(draggingSpy.count, 0)
            compare(activeSpy.count, 0)
        }

        // Remove selection
        mouseClick(tableView, 100, 100, Qt.LeftButton)
        verify(!tableView.selectionModel.hasSelection)
        compare(draggingSpy.count, 0)
        if (data.hasDelegate)
            compare(activeSpy.count, 2)
        else
            compare(activeSpy.count, 0)

        // Ensure that a drag doesn't start a selection
        mouseDrag(tableView, 1, 1, (cellWidth * 2) - 2, 1, Qt.LeftButton)
        verify(!tableView.selectionModel.hasSelection)
    }

    function test_pressAndHoldPlussShift_data() {
        return init_delegate_data_tag()
    }

    function test_pressAndHoldPlussShift(data) {
        let tableView = createTemporaryObject(data.viewComp, testCase)
        verify(tableView)
        let selectionRectangle = tableView.selectionRectangle
        verify(selectionRectangle)

        selectionRectangle.selectionMode = SelectionRectangle.Drag

        const invalidIndex = tableView.index(-1, -1)

        verify(!tableView.selectionModel.hasSelection)
        compare(tableView.selectionModel.currentIndex, invalidIndex)

        // select cell 0,0
        mouseClick(tableView, 1, 1, Qt.LeftButton)
        if (data.hasDelegate)
            compare(tableView.selectionModel.currentIndex, tableView.index(0, 0))
        else
            compare(tableView.selectionModel.currentIndex, invalidIndex)

        // do a long press on cell 1,0 while holding down Shift. This will
        // select both cells.
        mousePress(tableView, cellWidth + 1, 1, Qt.LeftButton, Qt.ShiftModifier)
        mouseRelease(tableView, cellWidth + 1, 1, Qt.LeftButton, Qt.ShiftModifier, 2000)
        compare(tableView.selectionModel.hasSelection, data.hasDelegate)
        if (tableView.selectionModel.hasSelection) {
            compare(tableView.selectionModel.selectedIndexes.length, 2)
            verify(tableView.selectionModel.isSelected(tableView.model.index(0, 0)))
            verify(tableView.selectionModel.isSelected(tableView.model.index(0, 1)))
        } else {
            verify(!selectionRectangle.active)
            verify(!topLeftHandle)
            verify(!bottomRightHandle)
        }
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

    function test_handleDragTopLeft_data() {
        return init_delegate_data_tag()
    }

    function test_handleDragTopLeft(data) {
        let tableView = createTemporaryObject(data.viewComp, testCase)
        verify(tableView)
        let selectionRectangle = tableView.selectionRectangle
        verify(selectionRectangle)

        selectionRectangle.selectionMode = SelectionRectangle.Drag

        verify(!tableView.selectionModel.hasSelection)
        // Select four cells in the middle
        mouseDrag(tableView, cellWidth + 1, cellHeight + 1, (cellWidth * 2) - 2, (cellHeight * 2) - 2, Qt.LeftButton)
        compare(tableView.selectionModel.hasSelection, data.hasDelegate)
        if (tableView.selectionModel.hasSelection) {
            compare(tableView.selectionModel.selectedIndexes.length, 4)
            for (let x = 1; x < 3; ++x) {
                for (let y = 1; y < 3; ++y) {
                    verify(tableView.selectionModel.isSelected(tableView.model.index(x, y)))
                }
            }
        } else {
            verify(!selectionRectangle.active)
            verify(!topLeftHandle)
            verify(!bottomRightHandle)
        }

        // Drag on the top left handle, so that the selection extends to cell 0, 0
        mouseDrag(tableView, cellWidth, cellHeight, -cellWidth / 2, -cellHeight / 2, Qt.LeftButton)
        compare(tableView.selectionModel.hasSelection, data.hasDelegate)
        if (tableView.selectionModel.hasSelection) {
            compare(tableView.selectionModel.selectedIndexes.length, 9)
            for (let x = 0; x < 3; ++x) {
                for (let y = 0; y < 3; ++y) {
                    verify(tableView.selectionModel.isSelected(tableView.model.index(x, y)))
                }
            }
        } else {
            verify(!selectionRectangle.active)
            verify(!topLeftHandle)
            verify(!bottomRightHandle)
        }
    }

    function test_handleDragBottomRight_shrink_data() {
        return init_delegate_data_tag()
    }

    function test_handleDragBottomRight_shrink(data) {
        let tableView = createTemporaryObject(data.viewComp, testCase)
        verify(tableView)
        let selectionRectangle = tableView.selectionRectangle
        verify(selectionRectangle)

        selectionRectangle.selectionMode = SelectionRectangle.Drag

        verify(!tableView.selectionModel.hasSelection)
        // Select four cells in the middle
        mouseDrag(tableView, cellWidth + 1, cellHeight + 1, (cellWidth * 2) - 2, (cellHeight * 2) - 2, Qt.LeftButton)
        compare(tableView.selectionModel.hasSelection, data.hasDelegate)
        if (tableView.selectionModel.hasSelection) {
            compare(tableView.selectionModel.selectedIndexes.length, 4)
            for (let x = 1; x < 3; ++x) {
                for (let y = 1; y < 3; ++y) {
                    verify(tableView.selectionModel.isSelected(tableView.model.index(x, y)))
                }
            }
        } else {
            verify(!selectionRectangle.active)
            verify(!topLeftHandle)
            verify(!bottomRightHandle)
        }

        // Drag on the bottom right handle, so that the selection shrinks to cell 1, 1
        mouseDrag(tableView, (cellWidth * 3) - 1, (cellHeight * 3) - 1, -cellWidth, -cellHeight, Qt.LeftButton)
        compare(tableView.selectionModel.hasSelection, data.hasDelegate)
        if (tableView.selectionModel.hasSelection) {
            compare(tableView.selectionModel.selectedIndexes.length, 1)
            verify(tableView.selectionModel.isSelected(tableView.model.index(1, 1)))
        } else {
            verify(!selectionRectangle.active)
            verify(!topLeftHandle)
            verify(!bottomRightHandle)
        }
    }

    function test_handleDragBottomRight_expand_data() {
        return init_delegate_data_tag()
    }

    function test_handleDragBottomRight_expand(data) {
        let tableView = createTemporaryObject(data.viewComp, testCase)
        verify(tableView)
        let selectionRectangle = tableView.selectionRectangle
        verify(selectionRectangle)

        selectionRectangle.selectionMode = SelectionRectangle.Drag

        verify(!tableView.selectionModel.hasSelection)
        // Select four cells in the middle
        mouseDrag(tableView, cellWidth + 1, cellHeight + 1, (cellWidth * 2) - 2, (cellHeight * 2) - 2, Qt.LeftButton)
        compare(tableView.selectionModel.hasSelection, data.hasDelegate)
        if (tableView.selectionModel.hasSelection) {
            compare(tableView.selectionModel.selectedIndexes.length, 4)
            for (let x = 1; x < 3; ++x) {
                for (let y = 1; y < 3; ++y) {
                    verify(tableView.selectionModel.isSelected(tableView.model.index(x, y)))
                }
            }
        } else {
            verify(!selectionRectangle.active)
            verify(!topLeftHandle)
            verify(!bottomRightHandle)
        }

        // Drag on the bottom right handle, so that the selection expands to cell 9 cells
        mouseDrag(tableView, cellWidth * 3, cellHeight * 3, 10, 10, Qt.LeftButton)
        compare(tableView.selectionModel.hasSelection, data.hasDelegate)
        if (tableView.selectionModel.hasSelection) {
            compare(tableView.selectionModel.selectedIndexes.length, 9)
            for (let x = 1; x < 4; ++x) {
                for (let y = 1; y < 4; ++y) {
                    verify(tableView.selectionModel.isSelected(tableView.model.index(x, y)))
                }
            }
        } else {
            verify(!selectionRectangle.active)
            verify(!topLeftHandle)
            verify(!bottomRightHandle)
        }
    }

    function test_programmatic_unselect_data() {
        return init_delegate_data_tag()
    }

    function test_programmatic_unselect(data) {
        // Check that the SelectionRectangle will be deactivated if the
        // selection is changed programatically.
        let tableView = createTemporaryObject(data.viewComp, testCase)
        verify(tableView)
        let selectionRectangle = tableView.selectionRectangle
        verify(selectionRectangle)

        selectionRectangle.selectionMode = SelectionRectangle.Drag

        verify(!tableView.selectionModel.hasSelection)
        mouseDrag(tableView, 1, 1, (cellWidth * 2) - 2, 1, Qt.LeftButton)
        compare(tableView.selectionModel.hasSelection, data.hasDelegate)
        if (tableView.selectionModel.hasSelection) {
            compare(tableView.selectionModel.selectedIndexes.length, 2)
            verify(selectionRectangle.active)
        } else {
            verify(!selectionRectangle.active)
        }

        tableView.selectionModel.clearSelection()
        verify(!selectionRectangle.active)
    }

    function test_extend_using_keyboard_data() {
        return init_delegate_data_tag()
    }

    function test_extend_using_keyboard(data) {
        // Check that the bottom-right selection handle will move if an
        // acitve selection is extended with the keyboard
        let tableView = createTemporaryObject(data.viewComp, testCase)
        verify(tableView)
        let selectionRectangle = tableView.selectionRectangle
        verify(selectionRectangle)

        selectionRectangle.bottomRightHandle = bottomRightHandleComp
        selectionRectangle.selectionMode = SelectionRectangle.Drag

        tableView.forceActiveFocus()
        verify(!tableView.selectionModel.hasSelection)
        mouseDrag(tableView, 1, 1, (cellWidth * 2) - 2, 1, Qt.LeftButton)
        compare(tableView.selectionModel.hasSelection, data.hasDelegate)
        if (tableView.selectionModel.hasSelection) {
            compare(tableView.selectionModel.selectedIndexes.length, 2)
            verify(selectionRectangle.active)
            verify(bottomRightHandle)
            compare(bottomRightHandle.x, (cellWidth * 2) - (bottomRightHandle.width / 2))
            compare(bottomRightHandle.y, cellHeight - (bottomRightHandle.height / 2))
        } else {
            verify(!selectionRectangle.active)
            verify(!topLeftHandle)
            verify(!bottomRightHandle)
        }

        keyPress(Qt.Key_Down, Qt.ShiftModifier)
        keyRelease(Qt.Key_Down, Qt.ShiftModifier)
        keyPress(Qt.Key_Right, Qt.ShiftModifier)
        keyRelease(Qt.Key_Right, Qt.ShiftModifier)
        if (data.hasDelegate) {
            verify(selectionRectangle.active)
            compare(tableView.selectionModel.selectedIndexes.length, 6)
            compare(bottomRightHandle.x, (cellWidth * 3) - (bottomRightHandle.width / 2))
            compare(bottomRightHandle.y, (cellHeight * 2) - (bottomRightHandle.height / 2))
        } else {
            verify(!selectionRectangle.active)
            verify(!topLeftHandle)
            verify(!bottomRightHandle)
        }
    }

    function test_inactive_after_unselecting_last_cell_data() {
        const mouse_ctrl_click = (item, x, y) => {
            mouseClick(item, x, y, Qt.LeftButton, Qt.ControlModifier)
        }
        const mouse_ctrl_press_and_hold = (item, x, y) => {
            const holdInterval = Qt.styleHints.mousePressAndHoldInterval + 1
            mousePress(item, x, y, Qt.LeftButton, Qt.ControlModifier)
            mouseRelease(item, x, y, Qt.LeftButton, Qt.ControlModifier, holdInterval)
        }

        return [
            { tag: "controlClick", doMouseEvent: mouse_ctrl_click },
            { tag: "pressAndHold", doMouseEvent: mouse_ctrl_press_and_hold },
        ]
    }

    function test_inactive_after_unselecting_last_cell(data) {
        // Deselecting the only selected cell should
        // make the selection rectangle inactive
        let tableView = createTemporaryObject(tableviewComp, testCase)
        verify(tableView)
        let selectionRectangle = tableView.selectionRectangle
        verify(selectionRectangle)

        selectionRectangle.topLeftHandle = topLeftHandleComp
        selectionRectangle.bottomRightHandle = bottomRightHandleComp
        selectionRectangle.selectionMode = SelectionRectangle.Drag

        const index_0_0 = tableView.model.index(0, 0)
        const index_0_1 = tableView.model.index(0, 1)

        const do_mouse_event_on = (index) => {
            const item = tableView.itemAtIndex(index)
            data.doMouseEvent(tableView, item.x + cellWidth / 2, item.y + 1)
        }

        do_mouse_event_on(index_0_0)
        do_mouse_event_on(index_0_1)
        verify(tableView.selectionModel.hasSelection)
        compare(tableView.selectionModel.selectedIndexes.length, 2)
        verify(tableView.selectionModel.isSelected(index_0_0))
        verify(tableView.selectionModel.isSelected(index_0_1))
        verify(selectionRectangle.active)
        verify(topLeftHandle && topLeftHandle.visible)
        verify(bottomRightHandle && bottomRightHandle.visible)

        do_mouse_event_on(index_0_0)
        do_mouse_event_on(index_0_1)
        verify(!tableView.selectionModel.hasSelection)
        compare(tableView.selectionModel.selectedIndexes.length, 0)
        verify(!selectionRectangle.active)
        verify(!topLeftHandle || !topLeftHandle.visible)
        verify(!bottomRightHandle || !bottomRightHandle.visible)
    }
}
