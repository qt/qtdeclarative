// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtQml.Models
import TestTableModel
import QtQuick.Controls
import Qt.labs.qmlmodels

ApplicationWindow {
    id: window
    width: 640
    height: 480
    visible: true

    property int selectedX: -1
    property int selectedY: -1

    Item {
        anchors.fill: parent

        Column {
            id: menu
            x: 2
            y: 2

            Row {
                spacing: 1
                Button {
                    text: "Add row"
                    onClicked: tableView.model.insertRows(selectedY, 1)
                }
                Button {
                    text: "Remove row"
                    onClicked: tableView.model.removeRows(selectedY, 1)
                }
                Button {
                    text: "Add column"
                    onClicked: tableView.model.insertColumns(selectedX, 1)
                }
                Button {
                    text: "Remove column"
                    onClicked: tableView.model.removeColumns(selectedX, 1)
                }
                SpinBox {
                    id: spaceSpinBox
                    from: -100
                    to: 100
                    value: 0
                }
            }

            Row {
                spacing: 1
                Button {
                    text: "fast-flick<br>center table"
                    onClicked: {
                        tableView.contentX += tableView.width * 1.2
                    }
                }
                Button {
                    text: "flick to end<br>center table"
                    onClicked: {
                        tableView.contentX = tableView.contentWidth - tableView.width
                    }
                }
                Button {
                    text: "fast-flick<br>headers"
                    onClicked: {
                        leftHeader.contentY += 1000
                        topHeader.contentX += 1000
                    }
                }
                Button {
                    text: "set/unset<br>master bindings"
                    onClicked: {
                        leftHeader.syncView = leftHeader.syncView ? null : tableView
                        topHeader.syncView = topHeader.syncView ? null : tableView
                    }
                }
                Button {
                    id: flickingMode
                    checkable: true
                    text: checked ? "Flickable" : "Scrollable"
                }
            }
            Text {
                text: "Selected: x:" + selectedX + ", y:" + selectedY
            }
        }

        TableView {
            id: topHeader
            objectName: "topHeader"
            anchors.left: tableView.left
            width: tableView.width
            anchors.top: menu.bottom
            height: 30
            clip: true
            ScrollBar.horizontal: ScrollBar {}

            model: TestTableModel {
                rowCount: 1
                columnCount: 200
            }

            delegate: Rectangle {
                implicitHeight: topHeader.height
                implicitWidth: 20
                color: "lightgray"
                Text { text: column }
            }

            columnSpacing: 1
            rowSpacing: 1

            syncView: tableView
            syncDirection: Qt.Horizontal
        }

        TableView {
            id: leftHeader
            objectName: "leftHeader"
            anchors.left: parent.left
            anchors.top: tableView.top
            height: tableView.height
            width: 30
            clip: true
            ScrollBar.vertical: ScrollBar {}

            model: TestTableModel {
                rowCount: 200
                columnCount: 1
            }

            delegate: Rectangle {
                implicitHeight: 50
                implicitWidth: leftHeader.width
                color: "lightgray"
                Text { text: row }
            }

            columnSpacing: 1
            rowSpacing: 1

            syncView: tableView
            syncDirection: Qt.Vertical
        }

        TableView {
            id: tableView
            objectName: "tableview"
            anchors.left: leftHeader.right
            anchors.right: parent.right
            anchors.top: topHeader.bottom
            anchors.bottom: parent.bottom
            width: 200
            clip: true
            delegate: tableViewDelegate
            columnSpacing: spaceSpinBox.value
            rowSpacing: spaceSpinBox.value
            interactive: flickingMode.checked

            columnWidthProvider: function(c) {
                if (c > 30)
                    return 100
            }

            ScrollBar.horizontal: ScrollBar {}
            ScrollBar.vertical: ScrollBar {}

            model: TestTableModel {
                rowCount: 200
                columnCount: 60
            }

            selectionModel: ItemSelectionModel {}
        }

        SelectionRectangle {
            target: tableView
        }

        Component {
            id: tableViewDelegate
            Rectangle {
                id: delegate
                implicitWidth: 50
                implicitHeight: 30
                border.width: row === selectedY && column == selectedX ? 2 : 0
                border.color: "darkgreen"
                color: selected ? "lightgreen" : "white"
                required property bool selected

                TapHandler {
                    onTapped: {
                        selectedX = column
                        selectedY = row
                    }
                }

                Text {
                    anchors.centerIn: parent
                    text: column + ", " + row
                }
            }
        }

    }

}
