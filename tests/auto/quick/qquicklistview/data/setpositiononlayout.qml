// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {
    width: 200
    height: 720

    ListView {
        anchors.fill: parent
        focus: true
        highlightMoveDuration: 200
        model: 50
        spacing: 10
        delegate: FocusScope {
            implicitHeight: col.height
            Column {
                id: col
                Text {
                    text: "YYYY"
                }
                ListView {
                    id: list
                    model: 1
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 250
                    orientation: ListView.Horizontal
                    delegate: Rectangle {
                        id: self
                        height: 250
                        width: 150
                        color: "blue"
                    }
                }
            }
        }
    }
}
