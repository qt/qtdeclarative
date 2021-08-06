/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
    property Item handle: null
    property bool handleWasDragged: false

    Component {
        id: handleComp
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

            Component.onCompleted: testCase.handle = handle
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
                rows: [
                    { "c1": "v1", "c2":"v2", "c3":"v3", "c4": "v4" },
                    { "c1": "v1", "c2":"v2", "c3":"v3", "c4": "v4" },
                    { "c1": "v1", "c2":"v2", "c3":"v3", "c4": "v4" },
                    { "c1": "v1", "c2":"v2", "c3":"v3", "c4": "v4" },
                ]
            }

            delegate: Rectangle {
                required property bool selected
                implicitWidth: cellWidth
                implicitHeight: cellHeight
                color: selected ? "lightblue" : "gray"
                Text { text: "cell" }
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
        id: signalSpy
        SignalSpy { }
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

        selectionRectangle.topLeftHandle = handleComp
        compare(selectionRectangle.topLeftHandle, handleComp)

        selectionRectangle.bottomRightHandle = handleComp
        compare(selectionRectangle.bottomRightHandle, handleComp)
    }

    function test_drag() {
        let tableView = createTemporaryObject(tableviewComp, testCase)
        verify(tableView)
        let selectionRectangle = tableView.selectionRectangle
        verify(selectionRectangle)

        selectionRectangle.selectionMode = SelectionRectangle.Drag

        let activeSpy = signalSpy.createObject(selectionRectangle, {target: selectionRectangle, signalName: "activeChanged"})
        let draggingSpy = signalSpy.createObject(selectionRectangle, {target: selectionRectangle, signalName: "draggingChanged"})
        verify(activeSpy.valid)
        verify(draggingSpy.valid)

        verify(!tableView.selectionModel.hasSelection)
        mouseDrag(tableView, 1, 1, (cellWidth * 2) - 2, 1, Qt.LeftButton)
        verify(tableView.selectionModel.hasSelection)
        compare(tableView.selectionModel.selectedIndexes.length, 2)
        verify(tableView.selectionModel.isSelected(tableView.model.index(0, 0)))
        verify(tableView.selectionModel.isSelected(tableView.model.index(0, 1)))

        compare(activeSpy.count, 1)
        compare(draggingSpy.count, 2)

        // Remove selection
        mouseClick(tableView, 1, 1, Qt.LeftButton)
        verify(!tableView.selectionModel.hasSelection)
        compare(draggingSpy.count, 2)
        compare(activeSpy.count, 2)

        // Ensure that a press and hold doesn't start a selection
        mousePress(tableView, 1, 1, Qt.LeftButton)
        mousePress(tableView, 1, 1, Qt.LeftButton, Qt.NoModifier, 1000)
        verify(!tableView.selectionModel.hasSelection)
    }

    function test_pressAndHold() {
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
        mouseDrag(tableView, cellWidth * 2, cellHeight * 2, -cellWidth / 2, -cellHeight / 2, Qt.LeftButton)
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
        mouseDrag(tableView, cellWidth * 3, cellHeight * 3, cellWidth * 4, cellHeight * 4, Qt.LeftButton)
        compare(tableView.selectionModel.selectedIndexes.length, 9)
        for (x = 1; x < 4; ++x) {
           for (y = 1; y < 4; ++y) {
               verify(tableView.selectionModel.isSelected(tableView.model.index(x, y)))
           }
        }
    }

}
