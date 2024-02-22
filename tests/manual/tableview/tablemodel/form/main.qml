// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12
import QtQuick.Window 2.12
import Qt.labs.qmlmodels 1.0

ApplicationWindow {
    id: window
    width: 800
    height: 800
    visible: true

    ColumnLayout {
        anchors.fill: parent

        TableView {
            id: tableView
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.horizontal: ScrollBar {}
            ScrollBar.vertical: ScrollBar {}

            Layout.minimumHeight: window.height / 2
            Layout.fillWidth: true
            Layout.fillHeight: true

            model: TableModel {
                TableModelColumn { display: "checked" }
                TableModelColumn { display: "amount" }
                TableModelColumn { display: "fruitType" }
                TableModelColumn { display: "fruitName" }
                TableModelColumn { display: "fruitPrice" }

                // One row = one type of fruit that can be ordered
                rows: [
                    {
                        // Each object (line) is one column,
                        // and each property in that object represents a role.
                        checked: false,
                        amount: 1,
                        fruitType: "Apple",
                        fruitName: "Granny Smith",
                        fruitPrice: 1.50
                    },
                    {
                        checked: true,
                        amount: 4,
                        fruitType: "Orange",
                        fruitName: "Navel",
                        fruitPrice: 2.50
                    },
                    {
                        checked: false,
                        amount: 1,
                        fruitType: "Banana",
                        fruitName: "Cavendish",
                        fruitPrice: 3.50
                    }
                ]
            }

            delegate: DelegateChooser {
                DelegateChoice {
                    column: 0
                    delegate: CheckBox {
                        objectName: "tableViewCheckBoxDelegate"
                        checked: model.display
                        onToggled: model.display = display
                    }
                }
                DelegateChoice {
                    column: 1
                    delegate: SpinBox {
                        objectName: "tableViewSpinBoxDelegate"
                        value: model.display
                        onValueModified: model.display = value
                    }
                }
                DelegateChoice {
                    delegate: TextField {
                        objectName: "tableViewTextFieldDelegate"
                        text: model.display
                        selectByMouse: true
                        implicitWidth: 140
                        onAccepted: model.display = text
                    }
                }
            }
        }

        TabBar {
            id: operationTabBar

            Layout.fillWidth: true
            Layout.preferredHeight: 40

            TabButton {
                text: "Append"
            }
            TabButton {
                text: "Clear"
            }
            TabButton {
                text: "Insert"
            }
            TabButton {
                text: "Move"
            }
            TabButton {
                text: "Remove"
            }
            TabButton {
                text: "Set"
            }
        }

        StackLayout {
            currentIndex: operationTabBar.currentIndex

            ColumnLayout {
                RowForm {
                    id: appendRowForm

                    Layout.fillHeight: true
                }

                Button {
                    text: "Append"

                    Layout.alignment: Qt.AlignRight

                    onClicked: tableView.model.appendRow(appendRowForm.inputAsRow())
                }
            }
            ColumnLayout {
                Button {
                    text: "Clear"
                    enabled: tableView.rows > 0

                    onClicked: tableView.model.clear()
                }
            }
            ColumnLayout {
                RowForm {
                    id: insertRowForm

                    Layout.fillHeight: true

                    Label {
                        text: "Insert index"
                    }
                    SpinBox {
                        id: insertIndexSpinBox
                        from: 0
                        to: tableView.rows
                    }
                }

                Button {
                    text: "Insert"

                    Layout.alignment: Qt.AlignRight

                    onClicked: tableView.model.insertRow(insertIndexSpinBox.value, insertRowForm.inputAsRow())
                }
            }
            GridLayout {
                columns: 2

                Label {
                    text: "Move from index"
                }
                SpinBox {
                    id: moveFromIndexSpinBox
                    from: 0
                    to: tableView.rows > 0 ? tableView.rows - 1 : 0
                }

                Label {
                    text: "Move to index"
                }
                SpinBox {
                    id: moveToIndexSpinBox
                    from: 0
                    to: tableView.rows > 0 ? tableView.rows - 1 : 0
                }

                Label {
                    text: "Rows to move"
                }
                SpinBox {
                    id: rowsToMoveSpinBox
                    from: 1
                    to: tableView.rows
                }

                Button {
                    text: "Move"
                    enabled: tableView.rows > 0

                    Layout.alignment: Qt.AlignRight
                    Layout.columnSpan: 2

                    onClicked: tableView.model.moveRow(moveFromIndexSpinBox.value, moveToIndexSpinBox.value, rowsToMoveSpinBox.value)
                }
            }
            GridLayout {
                Label {
                    text: "Remove index"
                }
                SpinBox {
                    id: removeIndexSpinBox
                    from: 0
                    to: tableView.rows > 0 ? tableView.rows - 1 : 0
                }

                Button {
                    text: "Remove"
                    enabled: tableView.rows > 0

                    Layout.alignment: Qt.AlignRight
                    Layout.columnSpan: 2

                    onClicked: tableView.model.removeRow(removeIndexSpinBox.value)
                }
            }
            ColumnLayout {
                RowForm {
                    id: setRowForm

                    Layout.fillHeight: true

                    Label {
                        text: "Set index"
                    }
                    SpinBox {
                        id: setIndexSpinBox
                        from: 0
                        to: tableView.rows > 0 ? tableView.rows - 1 : 0
                    }
                }

                Button {
                    text: "Set"

                    onClicked: tableView.model.setRow(setIndexSpinBox.value, setRowForm.inputAsRow());
                }
            }
        }
    }
}
