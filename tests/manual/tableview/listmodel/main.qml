// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick 2.12
import QtQuick.Window 2.3
import QtQml.Models 2.2

Window {
    id: window
    width: 640
    height: 480
    visible: true

    ListModel {
        id: listModel
        Component.onCompleted: {
            for (var i = 0; i < 30; ++i)
                listModel.append({"name" : i})
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 10
        color: "darkgray"

        TableView {
            id: tableView
            anchors.fill: parent
            anchors.margins: 1
            clip: true
            columnSpacing: 1
            rowSpacing: 1
            model: listModel
            delegate: Component {
                Rectangle {
                    id: tableDelegate
                    implicitWidth: 100
                    implicitHeight: 50

                    Text {
                        anchors.centerIn: parent
                        text: name + "\n[" + column + ", " + row + "]"
                    }
                }
            }
        }
    }
}
