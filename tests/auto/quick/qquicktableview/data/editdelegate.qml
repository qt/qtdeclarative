// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Window

Item {
    width: 640
    height: 450

    property alias tableView: tableView

    TableView {
        id: tableView
        anchors.fill: parent
        clip: true

        property Item editItem: null
        property var editIndex

        selectionModel: ItemSelectionModel {}

        delegate: Rectangle {
            implicitWidth: 100
            implicitHeight: 50

            required property bool editing

            Text {
                anchors.centerIn: parent
                text: display
            }

            TableView.editDelegate: TextInput {
                id: editRoot
                anchors.fill: parent
                text: display
                horizontalAlignment: TextInput.AlignHCenter
                verticalAlignment: TextInput.AlignVCenter
                activeFocusOnTab: true

                required property bool current
                required property bool selected
                required property bool editing

                Component.onCompleted: {
                    tableView.editItem = editRoot
                    tableView.editIndex = tableView.index(row, column)
                    selectAll()
                }

                Component.onDestruction: {
                    tableView.editItem = null
                    tableView.editIndex = tableView.index(-1, -1)
                }
            }
        }
    }

}
