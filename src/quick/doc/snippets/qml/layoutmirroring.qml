// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Rectangle {
    LayoutMirroring.enabled: true
    LayoutMirroring.childrenInherit: true

    width: 300; height: 50
    color: "yellow"
    border.width: 1

    Row {
        anchors { left: parent.left; margins: 5 }
        y: 5; spacing: 5

        Repeater {
            model: 5

            Rectangle {
                color: "red"
                opacity: (5 - index) / 5
                width: 40; height: 40

                Text {
                    text: index + 1
                    anchors.centerIn: parent
                }
            }
        }
    }
}
//![0]
