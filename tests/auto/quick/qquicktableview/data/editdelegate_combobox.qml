// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtQuick.Controls

Item {
    width: 640
    height: 450

    property alias tableView: tableView

    TableView {
        id: tableView
        anchors.fill: parent
        clip: true

        property Item editItem: null
        property Item focusItem: null
        property var editIndex
        property int commitCount: 0
        property int comboFocusCount: 0

        selectionModel: ItemSelectionModel {}

        delegate: Rectangle {
            implicitWidth: 100
            implicitHeight: 50

            required property bool editing

            Text {
                anchors.centerIn: parent
                text: display
                visible: !editing
            }

            TableView.editDelegate: FocusScope {
                id: editRoot
                anchors.fill: parent
                required property bool current
                required property bool selected
                required property bool editing

                TableView.onCommit: tableView.commitCount++;

                Component.onCompleted: {
                    tableView.editItem = editRoot
                    tableView.editIndex = tableView.index(row, column)
                }

                Component.onDestruction: {
                    tableView.editItem = null
                    tableView.editIndex = tableView.index(-1, -1)
                }

                ComboBox {
                    focus: true
                    model: 4
                    onActiveFocusChanged: if (activeFocus) tableView.comboFocusCount++;
                }
            }
        }
    }

}
