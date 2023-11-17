// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.qmlmodels

GroupBox {
    implicitWidth: parent.width
    implicitHeight: 180
    title: "Controls to generate"

    ScrollView {
        anchors.fill: parent
        TableView {
            id: tableView
            clip: true
            model: TableModel {
                id: tableModel
                TableModelColumn { display: "selected" }
                TableModelColumn { display: "control" }
                Component.onCompleted: {
                    let availableControls = bridge.availableControls()
                    let selectedControls = bridge.selectedControls()
                    let allSelected = selectedControls.length === availableControls.length
                    appendRow({ control: "All", selected: allSelected })
                    for (let i in availableControls) {
                        let name = availableControls[i]
                        let selected = selectedControls.includes(name)
                        appendRow({ control: name, selected: selected })
                    }

                }
            }
            delegate: DelegateChooser {
                DelegateChoice {
                    column: 0
                    delegate: CheckBox {
                        checked: model.display
                        onToggled: {
                            model.display = checked
                            if (model.row === 0) {
                                // "All" toggled
                                for (let r = 1; r < tableView.rows; ++r) {
                                    let name = tableModel.getRow(r).control
                                    let index = tableModel.index(r, 0)
                                    tableModel.setData(index, "display", checked)
                                    bridge.selectControl(name, checked)
                                }
                            } else {
                                let name = tableModel.getRow(model.row).control
                                let index = tableModel.index(model.row, 0)
                                tableModel.setData(index, "display", checked)
                                bridge.selectControl(name, checked)
                            }

                            let availableControls = bridge.availableControls()
                            let selectedControls = bridge.selectedControls()
                            let allSelected = selectedControls.length === availableControls.length
                            let allIndex = tableModel.index(0, 0)
                            tableModel.setData(allIndex, "display", allSelected)
                        }
                    }
                }
                DelegateChoice {
                    column: 1
                    delegate: Label {
                        text: model.display
                    }
                }
            }
        }
    }
}

